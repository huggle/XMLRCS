#!/usr/bin/python3
"""
es2r.py - Event Stream to Redis converter for XMLRCS

This script connects to Wikimedia's EventStream, processes recent changes,
converts them to XML format, and pushes them to Redis for further processing
by xmlrcsd.
"""
import sys
import time
import logging
import json
import redis
import os
import socket
import signal
import argparse
import traceback
from datetime import datetime
from sseclient import SSEClient as EventSource
from xml.sax.saxutils import quoteattr
from urllib.error import URLError, HTTPError
from requests.exceptions import RequestException, HTTPError as RequestsHTTPError

# Set up logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(sys.stdout),
        logging.FileHandler('es2r.log')
    ]
)
logger = logging.getLogger('es2r')

# Default configuration
DEFAULT_CONFIG = {
    'event_stream_url': 'https://stream.wikimedia.org/v2/stream/recentchange',
    'user_agent': 'XmlRcs/1.0 (https://github.com/huggle/XMLRCS) es2r-daemon',
    'redis_host': 'localhost',
    'redis_port': 6379,
    'redis_db': 0,
    'redis_key': 'rc',
    'heartbeat_interval': 60,  # seconds
    'reconnect_delay': 5,      # seconds
    'max_reconnect_delay': 300, # seconds
    'max_consecutive_errors': 10, # reconnect after this many consecutive errors
    'max_error_rate': 0.5,     # reconnect if error rate exceeds this (errors/events)
    'error_rate_window': 100,  # number of events to calculate error rate over
    'forced_reconnect_interval': 86400  # force reconnection every 24 hours
}

class EventStreamToRedis:
    """Main class to handle the event stream to Redis conversion."""
    
    def __init__(self, config=None):
        """Initialize the converter with configuration."""
        self.config = DEFAULT_CONFIG.copy()
        if config:
            self.config.update(config)
        
        self.redis_client = None
        self.event_source = None
        self.last_event_time = time.time()
        self.running = True
        self.events_processed = 0
        self.started_at = time.time()
        self.connect_attempt = 0
        
        # Connection health tracking
        self.consecutive_errors = 0
        self.recent_errors = []  # Track recent errors for rate calculation
        self.recent_events = []  # Track recent events for rate calculation
        self.connection_started_at = None
        
        # Setup signal handlers for graceful shutdown
        signal.signal(signal.SIGINT, self.signal_handler)
        signal.signal(signal.SIGTERM, self.signal_handler)
        
        # Initialize Redis connection
        self.init_redis()
        
        # Store PID for management
        self.redis_client.set("es2r.pid", int(os.getpid()))
        
    def signal_handler(self, sig, frame):
        """Handle termination signals."""
        logger.info(f"Received signal {sig}. Shutting down...")
        self.running = False
        if self.redis_client:
            self.redis_client.delete("es2r.pid")
        sys.exit(0)
        
    def init_redis(self):
        """Initialize Redis connection."""
        try:
            self.redis_client = redis.Redis(
                host=self.config['redis_host'],
                port=self.config['redis_port'],
                db=self.config['redis_db']
            )
            self.redis_client.ping()  # Test connection
            logger.info(f"Connected to Redis at {self.config['redis_host']}:{self.config['redis_port']}")
        except redis.ConnectionError as e:
            logger.error(f"Failed to connect to Redis: {e}")
            raise
            
    def heartbeat(self):
        """Update heartbeat in Redis and check for stalled connections."""
        current_time = time.time()
        time_since_last_event = current_time - self.last_event_time
        
        # Update heartbeat
        self.redis_client.set("es2r.heartbeat", int(current_time))
        self.redis_client.set("es2r.last_event", int(self.last_event_time))
        self.redis_client.set("es2r.events_processed", self.events_processed)
        self.redis_client.set("es2r.uptime", int(current_time - self.started_at))
        self.redis_client.set("es2r.consecutive_errors", self.consecutive_errors)
        
        # Check if connection might be stalled
        if time_since_last_event > self.config['heartbeat_interval'] * 2:
            logger.warning(f"No events received for {time_since_last_event:.1f} seconds. Connection may be stalled.")
            return False
            
        # Check if we need a forced reconnection (every 24 hours)
        if self.config['forced_reconnect_interval'] > 0 and self.connection_started_at and (current_time - self.connection_started_at) > self.config['forced_reconnect_interval']:
            logger.info(f"Forcing reconnection after {(current_time - self.connection_started_at)/3600:.1f} hours uptime")
            return False
            
        return True
    
    def record_error(self):
        """Record an error and check if we should reconnect."""
        current_time = time.time()
        self.consecutive_errors += 1
        self.recent_errors.append(current_time)
        
        # Keep only recent errors within the window
        cutoff_time = current_time - 60  # Keep last minute of errors
        self.recent_errors = [t for t in self.recent_errors if t > cutoff_time]
        
        # Check consecutive error threshold
        if self.consecutive_errors >= self.config['max_consecutive_errors']:
            logger.error(f"Reached {self.consecutive_errors} consecutive errors. Forcing reconnection.")
            return True
            
        # Check error rate if we have enough events
        if len(self.recent_events) >= 10:
            # Keep only recent events within the window
            cutoff_time = current_time - 60
            self.recent_events = [t for t in self.recent_events if t > cutoff_time]
            
            # Calculate error rate over recent window
            if len(self.recent_events) > 0:
                error_rate = len(self.recent_errors) / (len(self.recent_events) + len(self.recent_errors))
                if error_rate > self.config['max_error_rate']:
                    logger.error(f"Error rate {error_rate:.2%} exceeds threshold {self.config['max_error_rate']:.2%}. Forcing reconnection.")
                    return True
        
        return False
    
    def record_success(self):
        """Record a successful event processing."""
        current_time = time.time()
        self.consecutive_errors = 0  # Reset on success
        self.recent_events.append(current_time)
        
        # Keep only recent events within the window
        cutoff_time = current_time - 60
        self.recent_events = [t for t in self.recent_events if t > cutoff_time]
        
    def insert_to_redis(self, wiki, xml):
        """Insert XML data into Redis."""
        try:
            result = wiki + "|" + xml
            self.redis_client.rpush(self.config['redis_key'], result)
            return True
        except redis.RedisError as e:
            logger.error(f"Redis error: {e}")
            # Try to reconnect
            try:
                self.init_redis()
                # Retry insertion
                result = wiki + "|" + xml
                self.redis_client.rpush(self.config['redis_key'], result)
                return True
            except redis.RedisError as e2:
                logger.error(f"Redis reconnection failed: {e2}")
                return False
                
    def format_xml(self, change):
        """Format a change event as XML."""
        try:
            # Validate critical fields first
            if 'wiki' not in change:
                logger.error(f"Missing critical field 'wiki' in change data. Keys present: {list(change.keys())}")
                return None
            if 'server_name' not in change:
                logger.error(f"Missing critical field 'server_name' in change data")
                return None
                
            rev_id = ''
            patrolled = False
            length_n = 0
            length_o = 0
            minor = False
            old = ''
            
            # Extract revision information
            if 'revision' in change:
                rev_id = f'revid="{change["revision"]["new"]}" '
                if 'old' in change['revision']:
                    old = f'oldid="{change["revision"]["old"]}" '
                    
            # Extract other properties
            if 'patrolled' in change:
                patrolled = change['patrolled']
            if 'minor' in change:
                minor = change['minor']
            if 'length' in change:
                length_n = change['length']['new']
                if 'old' in change['length']:
                    length_o = change['length']['old']
                    
            # Build XML
            result = f'<edit wiki="{change["wiki"]}" '
            result += f'server_name="{change["server_name"]}" '
            result += rev_id + old
            result += f'summary={quoteattr(change.get("comment", ""))} '
            result += f'title={quoteattr(change["title"])} '
            result += f'namespace="{change["namespace"]}" '
            result += f'user={quoteattr(change["user"])} '
            result += f'bot="{change["bot"]}" '
            result += f'patrolled="{patrolled}" '
            result += f'minor="{minor}" '
            result += f'type={quoteattr(change["type"])} '
            result += f'length_new="{length_n}" '
            result += f'length_old="{length_o}" '
            
            # Add log information if present
            if change['type'] == 'log':
                if 'log_id' in change:
                    result += f'log_id="{change["log_id"]}" '
                if 'log_type' in change:
                    result += f'log_type={quoteattr(change["log_type"])} '
                if 'log_action' in change:
                    result += f'log_action={quoteattr(change["log_action"])} '
                if 'log_action_comment' in change:
                    result += f'log_action_comment={quoteattr(change["log_action_comment"])} '
                    
            result += f'timestamp="{change["timestamp"]}">'
            result += '</edit>'
            
            return result
        except KeyError as e:
            logger.error(f"Missing key in change data: {e}")
            logger.debug(f"Change data: {change}")
            return None
            
    def process_event(self, event):
        """Process a single event from the stream."""
        if event.event != 'message':
            return
        
        # Skip empty or whitespace-only data
        if not event.data or not event.data.strip():
            return
            
        try:
            change = json.loads(event.data)
            
            # Update last event time
            self.last_event_time = time.time()
            self.events_processed += 1
            
            # Format as XML and insert to Redis
            xml = self.format_xml(change)
            if xml:
                self.insert_to_redis(change['server_name'], xml)
                self.record_success()  # Record successful processing
            else:
                # format_xml returned None due to missing critical fields
                if self.record_error():
                    raise ConnectionError("Too many formatting errors, reconnecting")
                
        except ValueError as e:
            logger.warning(f"Invalid JSON in event data: {e}")
            logger.debug(f"Event type: {event.event}, Event data: '{event.data[:200] if len(event.data) > 200 else event.data}...' (truncated)")
            # JSON parsing errors indicate corrupted stream
            if self.record_error():
                raise ConnectionError("Too many JSON parsing errors, reconnecting")
        except KeyError as e:
            logger.error(f"Missing key in event: {e}")
            if self.record_error():
                raise ConnectionError("Too many missing key errors, reconnecting")
        except Exception as e:
            logger.error(f"Error processing event: {e}")
            logger.debug(traceback.format_exc())
            if self.record_error():
                raise ConnectionError("Too many processing errors, reconnecting")
            
    def connect_to_event_stream(self):
        """Connect to the EventStream service."""
        try:
            logger.info(f"Connecting to EventStream at {self.config['event_stream_url']}")
            self.connect_attempt += 1
            
            # Set proper headers for Wikimedia EventStream API
            headers = {
                'User-Agent': self.config['user_agent'],
                'Accept': 'text/event-stream',
                'Cache-Control': 'no-cache'
            }
            
            logger.debug(f"Using headers: {headers}")
            self.event_source = EventSource(self.config['event_stream_url'], headers=headers)
            self.connect_attempt = 0  # Reset on successful connection
            self.connection_started_at = time.time()  # Track connection start time
            self.consecutive_errors = 0  # Reset error counters
            self.recent_errors = []
            self.recent_events = []
            logger.info("Successfully connected to EventStream")
            return True
        except (HTTPError, RequestsHTTPError) as e:
            delay = min(
                self.config['reconnect_delay'] * (2 ** min(self.connect_attempt, 6)),
                self.config['max_reconnect_delay']
            )
            if hasattr(e, 'code'):
                status_code = e.code
            elif hasattr(e, 'response') and e.response is not None:
                status_code = e.response.status_code
            else:
                status_code = "unknown"
            logger.error(f"HTTP error connecting to EventStream (status {status_code}): {e}. Check User-Agent header. Retrying in {delay} seconds...")
            logger.debug(f"Using User-Agent: {self.config['user_agent']}")
            time.sleep(delay)
        except (URLError, RequestException, ConnectionError, socket.error) as e:
            delay = min(
                self.config['reconnect_delay'] * (2 ** min(self.connect_attempt, 6)),
                self.config['max_reconnect_delay']
            )
            logger.error(f"Failed to connect to EventStream: {e}. Retrying in {delay} seconds...")
            time.sleep(delay)
            return False
            
    def run(self):
        """Main processing loop."""
        logger.info("Starting EventStream to Redis converter")
        
        next_heartbeat = time.time() + self.config['heartbeat_interval']
        
        while self.running:
            try:
                # Try to connect if not connected
                if not self.event_source:
                    if not self.connect_to_event_stream():
                        continue
                
                # Process events with timeout check
                event_iterator = iter(self.event_source)
                
                while self.running:
                    # Check heartbeat
                    if time.time() >= next_heartbeat:
                        if not self.heartbeat():
                            # Connection might be stalled, reconnect
                            logger.info("Reconnecting due to possible stalled connection or forced reconnection interval")
                            self.event_source = None
                            break
                        next_heartbeat = time.time() + self.config['heartbeat_interval']
                    
                    # Get next event with timeout
                    try:
                        event = next(event_iterator)
                        self.process_event(event)
                    except StopIteration:
                        logger.warning("EventStream connection closed by server")
                        self.event_source = None
                        break
                    except ConnectionError as e:
                        # Raised by process_event when error threshold exceeded
                        logger.error(f"Connection quality degraded: {e}")
                        self.event_source = None
                        break
                    
            except (URLError, RequestException, ConnectionError, socket.error) as e:
                logger.error(f"Connection error in main loop: {e}")
                self.event_source = None
                time.sleep(self.config['reconnect_delay'])
                
            except Exception as e:
                logger.error(f"Unexpected error in main loop: {e}")
                logger.debug(traceback.format_exc())
                self.event_source = None
                time.sleep(self.config['reconnect_delay'])
                
def parse_arguments():
    """Parse command line arguments."""
    parser = argparse.ArgumentParser(description='Event Stream to Redis converter for XMLRCS')
    parser.add_argument('--redis-host', default='localhost', help='Redis host')
    parser.add_argument('--redis-port', type=int, default=6379, help='Redis port')
    parser.add_argument('--redis-db', type=int, default=0, help='Redis database')
    parser.add_argument('--redis-key', default='rc', help='Redis key for pushing data')
    parser.add_argument('--url', default=DEFAULT_CONFIG['event_stream_url'], help='EventStream URL')
    parser.add_argument('--user-agent', default=DEFAULT_CONFIG['user_agent'], help='User-Agent header for EventStream requests')
    parser.add_argument('--debug', action='store_true', help='Enable debug logging')
    return parser.parse_args()

if __name__ == '__main__':
    args = parse_arguments()
    
    # Configure logging level
    if args.debug:
        logger.setLevel(logging.DEBUG)
    
    # Create configuration from arguments
    config = {
        'event_stream_url': args.url,
        'user_agent': args.user_agent,
        'redis_host': args.redis_host,
        'redis_port': args.redis_port,
        'redis_db': args.redis_db,
        'redis_key': args.redis_key
    }
    
    # Create and run the converter
    converter = EventStreamToRedis(config)
    try:
        converter.run()
    except KeyboardInterrupt:
        logger.info("Shutting down...")
    except Exception as e:
        logger.critical(f"Fatal error: {e}")
        logger.debug(traceback.format_exc())
        sys.exit(1)
