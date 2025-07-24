# EventStream to Redis (es2r)

This component of XMLRCS connects to the Wikimedia EventStream API, processes recent changes, converts them to XML format, and pushes them to a Redis queue for further processing by xmlrcsd.

## Features

- Robust connection to Wikimedia's EventStream API
- Automatic reconnection with exponential backoff
- Comprehensive error handling and recovery
- Connection stall detection and recovery
- Performance and health monitoring via Redis
- Flexible configuration via command line arguments

## Usage

```bash
python es2r.py [options]
```

### Command Line Options

- `--redis-host HOSTNAME` - Redis host (default: localhost)
- `--redis-port PORT` - Redis port (default: 6379)
- `--redis-db DB` - Redis database (default: 0)
- `--redis-key KEY` - Redis key for pushing data (default: rc)
- `--url URL` - EventStream URL (default: https://stream.wikimedia.org/v2/stream/recentchange)
- `--debug` - Enable debug logging

## Monitoring

The script stores the following monitoring information in Redis:

- `es2r.pid` - Process ID
- `es2r.heartbeat` - Timestamp of last heartbeat
- `es2r.last_event` - Timestamp of last received event
- `es2r.events_processed` - Total number of events processed
- `es2r.uptime` - Uptime in seconds

## Error Recovery

The script implements multiple mechanisms to ensure reliability:

1. **Automatic reconnection**: If the connection to EventStream fails, the script will attempt to reconnect with exponential backoff.
2. **Stall detection**: If no events are received for a certain period, the script will assume the connection is stalled and reconnect.
3. **Redis failure handling**: If Redis becomes unavailable, the script will attempt to reconnect.
4. **Signal handling**: The script responds correctly to SIGINT and SIGTERM signals for graceful shutdown.

## Requirements

- Python 3.6+
- sseclient
- redis
- requests
