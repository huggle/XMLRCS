#!/usr/bin/python
import logging
import socketIO_client
import json
import redis
from xml.sax.saxutils import quoteattr

#DEBUG:socketIO_client.transports:[packet received] 5::/rc:{"args":[{"comment":"","wiki":"metawiki","server_name":"meta.wikimedia.beta.wmflabs.org","title":"Huggle/Config","timestamp":1407505120,"server_script_path":"/w","namespace":0,"server_url":"http://meta.wikimedia.beta.wmflabs.org","length":{"new":1232,"old":1226},"user":"90.183.23.27","bot":false,"pat
#olled":false,"type":"edit","id":1912,"minor":false,"revision":{"new":1136,"old":1107}}],"name":"change"} 
#90.183.23.27 edited Huggle/Config


#logging.basicConfig(level='DEBUG')
rs = redis.Redis('localhost') 

def insert_to_redis(wiki, xml):
    result = wiki + "|" + xml
    rs.rpush('rc', result);
    return True;

class WikiNamespace(socketIO_client.BaseNamespace):
    def on_change(self, change):
        rev_id = ''
        patrolled=False
        length_n=0
        length_o=0
        minor=False
        old = ''
        if ('revision' in change):
            rev_id = 'revid="' + str(change['revision']['new']) + '" '
            old = 'oldid="' + str(change['revision']['old']) + '" '
        if ('patrolled' in change):
            patrolled = change['patrolled']
        if ('minor' in change):
            minor = change['minor']
        if ('length' in change):
            length_n = change['length']['new']
            length_o = change['length']['old']
        result = '<edit wiki="' + change['wiki'] + '" '
        result += 'server_name="' + change['server_name'] + '" ' 
        result += rev_id + old
        result += 'summary=' + quoteattr(change['comment']) + ' '
        result += 'title=' +  quoteattr(change['title']) + ' '
        result += 'namespace="' + str(change['namespace']) + '" '
        result += 'user=' + quoteattr(change['user']) + ' '
        result += 'bot="' + str(change['bot']) + '" '
        result += 'patrolled="' + str(patrolled) + '" '
        result += 'minor="' + str(minor) + '" '
        result += 'type=' + quoteattr(change['type']) + ' '
        result += 'length_new="' + str(length_n) + '" '
        result += 'length_old="' + str(length_o) + '" '
        result += 'timestamp="' + str(change['timestamp']) + '">'
        result += '</edit>'
        insert_to_redis(change['server_name'], result)
#        print '%(user)s edited %(title)s' % change
 
    def on_connect(self):
        self.emit('subscribe', '*')
 
 
socketIO = socketIO_client.SocketIO('stream.wikimedia.org', 80)
socketIO.define(WikiNamespace, '/rc')
 
socketIO.wait()
