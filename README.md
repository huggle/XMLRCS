XMLRCS
======

Recent changes xml stream for MediaWiki websites.

This is a simple TCP/IP provider of recent changes for MediaWiki. It basically takes the extremely complicated WebSocket IO JSON provider that is now supported by WMF and push the data to redis in XML format. The daemon (xmlrcsd) is then listening on port 8822 for any incoming connections and allow them to retrieve the stream in a simple way that can be easily parsed in any programming language with no need to support WebSockets or JSON.

There is working instance at huggle-rc.wmflabs.org which you can try yourself, listening on port 8822

For example:
```
telnet huggle-rc.wmflabs.org 8822
S all
exit
```

Please see https://wikitech.wikimedia.org/wiki/XmlRcs

Technical details
=================

Incoming text should be encoded in ASCII (it contains only URL's so it shouldn't be a problem) while outgoing text produced by XmlRcs is UTF8. This will likely change in a future and all text will be UTF8.

This XML system contains of 3 components:

* Convertor written in python which connects to WMF feed and push data to redis (pretty simple thing)
* Redis server
* Daemon written in C++ which handles the incoming connections (high speed, resource effective)
