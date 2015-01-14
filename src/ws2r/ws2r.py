#!/usr/bin/python
import logging
import socketIO_client
import json
import redis
from xml.sax.saxutils import quoteattr

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
    def on_reconnect(self, *args):
        self.emit('subscribe', '*') 
    def on_connect(self):
        self.emit('subscribe', '*')
 
 
socketIO = socketIO_client.SocketIO('stream.wikimedia.org', 80)
socketIO.define(WikiNamespace, '/rc')
 
socketIO.wait()
