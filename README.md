XMLRCS
======

Recent changes xml stream for mediawiki

This is a simple XML provider for recent changes for MediaWiki. It basically takes the extremely complicated WebSocket IO JSON provider that is now supported by WMF and push the data to redis in XML format. The daemon (xmlrcsd) is then listening on port 8822 for any incoming connections and allow them to retrieve the stream in a simple way.

Protocol
=========

This is how you can use xmlrcs to get recent changes stream, you just need to open a tcp connection on port 8822, for example use telnet localhost 8822, then you can use one of these commands:

## exit
Terminate connection
## S (url of wiki)
subscribe to a wiki, for example:

S en.wikipedia.org

you can also use * to subscribe to all wikis
## stat
Display various system information useful for debugging
## version
Display version of daemon
