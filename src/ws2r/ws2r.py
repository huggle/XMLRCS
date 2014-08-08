#!/usr/bin/python
import logging
import socketIO_client
import json
import redis

#DEBUG:socketIO_client.transports:[packet received] 5::/rc:{"args":[{"comment":"","wiki":"metawiki","server_name":"meta.wikimedia.beta.wmflabs.org","title":"Huggle/Config","timestamp":1407505120,"server_script_path":"/w","namespace":0,"server_url":"http://meta.wikimedia.beta.wmflabs.org","length":{"new":1232,"old":1226},"user":"90.183.23.27","bot":false,"pat
r 
#olled":false,"type":"edit","id":1912,"minor":false,"revision":{"new":1136,"old":1107}}],"name":"change"} 
#90.183.23.27 edited Huggle/Config


logging.basicConfig(level='DEBUG')
rs = redis.Redis('localhost') 

def insert_to_redis(wiki, xml):
    rs.rpush('rc', wiki + '|' + xml);
    return true;

class WikiNamespace(socketIO_client.BaseNamespace):
    def on_change(self, change):
        insert_to_redis(change['server_name'], '<edit></edit>');
        print '%(user)s edited %(title)s' % change
 
    def on_connect(self):
        self.emit('subscribe', '*')
 
 
socketIO = socketIO_client.SocketIO('stream.wmflabs.org', 80)
socketIO.define(WikiNamespace, '/rc')
 
socketIO.wait()
