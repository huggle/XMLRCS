# XmlRcs AI Coding Assistant Instructions

## System Architecture Overview

XmlRcs is a high-performance MediaWiki recent changes streaming service with a 3-component pipeline:

1. **es2r** (EventStream to Redis) - Python daemon that connects to Wikimedia's EventStream API, converts JSON to XML, and pushes to Redis
2. **Redis** - Message queue buffer between components  
3. **xmlrcsd** - Multi-threaded C++ TCP server (port 8822) that serves XML streams to clients

**Data Flow**: Wikimedia EventStream → es2r → Redis → xmlrcsd → TCP clients

## Critical Development Workflows

### Building Components
- **xmlrcsd**: Use CMake (`cmake . && make`) - includes bundled hiredis
- **C# client**: Build with MSBuild/Visual Studio - targets .NET Framework 2.0
- **Python components**: Direct execution, install deps with `pip install sseclient redis requests`

### Production Deployment
- Install via systemd using `src/init/install_systemd.sh` 
- Services run as `xmlrcs` user in `/opt/xmlrcs`
- Both services depend on Redis and start in dependency order
- Monitor via `journalctl -u es2r -f` and `journalctl -u xmlrcsd -f`

### Testing Local Instance
```bash
telnet huggle-rc.wmflabs.org 8822
S all
exit
```

## Project-Specific Patterns

### XML Format Convention
All components must maintain the specific XML schema:
```xml
<edit wiki="en.wikipedia.org" server_name="en.wikipedia.org" revid="123" 
      summary="edit summary" title="Article Name" namespace="0" 
      user="Username" bot="false" patrolled="true" minor="false" 
      type="edit" length_new="1234" length_old="1200" timestamp="1234567890">
</edit>
```

### Redis Integration Pattern
- Key naming: `es2r.*` for monitoring (pid, heartbeat, last_event, events_processed, uptime)
- Data format: `{wiki}|{xml}` pushed to `rc` key with `RPUSH`
- Connection handling: Auto-reconnect with exponential backoff in Python, polling loop in C++

### Error Recovery Mechanisms
- **es2r**: Stall detection (no events for 2x heartbeat interval), auto-restart via SIGKILL
- **xmlrcsd**: Client timeout (80 seconds), Redis polling with empty queue warnings
- **C# client**: Ping/pong heartbeat system, automatic reconnection support

### Multi-Language Client Protocol
TCP protocol uses simple commands:
- `S {wiki}` - Subscribe to wiki (use "all" for everything)  
- `D {wiki}` - Unsubscribe from wiki
- `ping`/`pong` - Heartbeat mechanism
- `exit` - Clean disconnect

### Configuration Patterns
- **Python**: Command-line args with defaults in `DEFAULT_CONFIG` dict
- **C++**: Static `Configuration` class with global state
- **C#**: Static `Configuration` class with connection timeouts and server settings

## Integration Points

### EventStream API Specifics
- URL: `https://stream.wikimedia.org/v2/stream/recentchange`
- Uses SSE (Server-Sent Events) with JSON payloads
- Requires robust reconnection due to network instability
- Legacy `ws2r.py` supports older WebSocket API (deprecated)

### Redis Schema
- Queue key: `rc` (configurable)
- Monitoring keys: `es2r.{pid,heartbeat,last_event,events_processed,uptime}`
- Connection: Default localhost:6379, configurable per component

### Client Development Guidelines
- Implement ping/pong heartbeat (60s intervals)
- Handle `<fatal>` vs `<error>` messages appropriately  
- Parse XML attributes case-sensitively
- Support auto-reconnection for production robustness
- See `clients/c#/XmlRcs/Provider.cs` for reference implementation

## Component-Specific Notes

### es2r.py Monitoring
Uses comprehensive Redis-based health monitoring. Check these keys to diagnose issues:
- Process health: `es2r.pid`, `es2r.heartbeat`  
- Data flow: `es2r.last_event`, `es2r.events_processed`
- Performance: `es2r.uptime`

### xmlrcsd Performance
- Multi-threaded with dedicated killer thread for timeouts
- Embedded hiredis for minimal dependencies
- Monitors Redis queue emptiness and can auto-restart es2r via PID
- Handles multiple concurrent client connections efficiently

### C# Client Features
- Thread pool management via custom `ThreadPool` class
- Event-driven architecture with typed `EventArgs`
- Automatic XML parsing with strongly-typed `RecentChange` objects
- Built-in subscription management and auto-resubscription support