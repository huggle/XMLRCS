#!/usr/bin/python
import logging
import socketIO_client
import json
import redis
import html

#DEBUG:socketIO_client.transports:[packet received] 5::/rc:{"args":[{"comment":"","wiki":"metawiki","server_name":"meta.wikimedia.beta.wmflabs.org","title":"Huggle/Config","timestamp":1407505120,"server_script_path":"/w","namespace":0,"server_url":"http://meta.wikimedia.beta.wmflabs.org","length":{"new":1232,"old":1226},"user":"90.183.23.27","bot":false,"pat
#olled":false,"type":"edit","id":1912,"minor":false,"revision":{"new":1136,"old":1107}}],"name":"change"} 
#90.183.23.27 edited Huggle/Config


#logging.basicConfig(level='DEBUG')
rs = redis.Redis('localhost') 

def insert_to_redis(wiki, xml):
    result = wiki + "|" + xml
    print(result)
    #rs.rpush('rc', wiki + '|' + xml);
    return True;

class WikiNamespace(socketIO_client.BaseNamespace):
    def on_change(self, change):
        rev_id = ''
        if ('revision' in change):
            rev_id = 'revid="' + str(change['revision']['new']) + '" '
        insert_to_redis(change['server_name'], '<edit wiki="' + html.escape(change['wiki']) + '" server_name="' + change['server_name'] + '" ' + rev_id + 'summary="' + change['comment'] + '" title="' +  change['title'] + '" timestamp="' + change['timestamp'] + '"></edit>');
#        print '%(user)s edited %(title)s' % change
 
    def on_connect(self):
        self.emit('subscribe', '*')
 
 
socketIO = socketIO_client.SocketIO('stream.wikimedia.org', 80)
socketIO.define(WikiNamespace, '/rc')
 
socketIO.wait()
