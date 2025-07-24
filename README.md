XMLRCS
======

Recent changes XML stream for MediaWiki websites.

This is a simple TCP/IP provider of recent changes for MediaWiki. It fetches data from Wikimedia's EventStream API, converts the JSON data to XML format, and pushes it to Redis. The daemon (xmlrcsd) then listens on port 8822 for incoming connections and allows clients to retrieve the stream in a simple format that can be easily parsed in any programming language without needing to support EventStream or JSON.

There is a working instance at huggle-rc.wmflabs.org which you can try yourself, listening on port 8822:

```
telnet huggle-rc.wmflabs.org 8822
S all
exit
```

Please see https://wikitech.wikimedia.org/wiki/XmlRcs for more information.

Technical details
=================

Outgoing text produced by XmlRcs is UTF-8 encoded, allowing for proper handling of international characters.

This XML system consists of 3 components:

* **es2r (EventStream to Redis)**: A Python script that connects to Wikimedia's EventStream API, processes the recent changes, converts them to XML format, and pushes them to Redis. Features automatic reconnection, error handling, and monitoring capabilities.
* **Redis server**: Acts as a message queue between the EventStream connector and the TCP server.
* **xmlrcsd**: A high-performance C++ daemon that handles incoming client connections and serves the XML stream to clients.
