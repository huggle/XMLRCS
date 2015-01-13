XMLRCS
======

Recent changes xml stream for mediawiki

This is a simple XML provider for recent changes for MediaWiki. It basically takes the extremely complicated WebSocket IO JSON provider that is now supported by WMF and push the data to redis in XML format. The daemon (xmlrcsd) is then listening on port 8822 for any incoming connections and allow them to retrieve the stream in a simple way that can be easily parsed in any programming language with no need to support WebSockets or JSON.

Protocol
=========

This is how you can use xmlrcs to get recent changes stream, you just need to open a tcp connection on port 8822, for example use telnet localhost 8822, then you can use one of these commands:

## exit
Terminate connection
## S (url of wiki)
subscribe to a wiki, for example:

S en.wikipedia.org

you can also use * to subscribe to all wikis.

Response: OK on success or ERROR with reason on error
## ping
Respond with "pong" used to test if connection is still alive or not
## stat
Display various system information useful for debugging
## version
Display version of daemon

Output
======

In case that you are subscribed to a wiki and there is a change, daemon will send you a 1 line of text in this format:
url|xml

for example:
```
en.wikipedia.org|<edit wiki="enwiki" server_name="en.wikipedia.org" revid="642306515" oldid="642305956" summary="" title="Thadiya (Jodhpur)" namespace="0" user="Teju Suthar" bot="False" patrolled="False" minor="True" length_new="551" length_old="547" timestamp="1421155443"></edit>
en.wikipedia.org|<newpage wiki="en.wikipedia.org" timestamp="4325235" user="Cookie" page="Main_page">summary</newpage>
```

Technical details
=================

This XML system contains of 3 components:

* Convertor written in python which connects to WMF feed and push data to redis (pretty simple thing)
* Redis server
* Daemon written in C++ which handles the incoming connections (high speed, resource effective)
