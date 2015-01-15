XMLRCS
======

Recent changes xml stream for mediawiki

This is a simple XML provider for recent changes for MediaWiki. It basically takes the extremely complicated WebSocket IO JSON provider that is now supported by WMF and push the data to redis in XML format. The daemon (xmlrcsd) is then listening on port 8822 for any incoming connections and allow them to retrieve the stream in a simple way that can be easily parsed in any programming language with no need to support WebSockets or JSON.

There is working instance at huggle-rc.wmflabs.org which you can try yourself

Protocol
=========

This is how you can use xmlrcs to get recent changes stream, you just need to open a tcp connection on port 8822, for example use telnet localhost 8822, then you can use one of these commands:

## exit
Terminate connection
## S (url of wiki)
subscribe to a wiki, for example:

S en.wikipedia.org

you can also use * to subscribe to all wikis.

Response: xml node "ok" on success or ERROR with reason on error
## ping
Respond with xml node "pong" used to test if connection is still alive or not
## stat
Display various system information useful for debugging
## version
Display version of daemon

Output
======

In case that you are subscribed to a wiki and there is a change, daemon will send you a 1 line of text in this format:

```
<edit wiki="enwiki" server_name="en.wikipedia.org" revid="642306515" oldid="642305956" summary="" title="Thadiya (Jodhpur)" namespace="0" user="Teju Suthar" bot="False" patrolled="False" minor="True" length_new="551" length_old="547" timestamp="1421155443"></edit>
```

Known standard attributes of each edit node:
* wiki: name of a wiki as a shortcut (enwiki)
* server_name: fqdn of server (en.wikipedia.org)
* revid: revision id (54635262)
* oldid: previous revision id (5635323)
* summary: summary of edit
* title: name of page
* user: name of user
* bot
* patrolled
* minor
* type: type of edit (edit is regular edit, new is a newpage)
* length_new: size of new edit
* length_old: size of old edit
* timestamp

Most of these attributes are present in every node, but they are not guaranteed to be there, so you should verify if they are present before reading them.


Technical details
=================

This XML system contains of 3 components:

* Convertor written in python which connects to WMF feed and push data to redis (pretty simple thing)
* Redis server
* Daemon written in C++ which handles the incoming connections (high speed, resource effective)
