#!/usr/bin/python
import sys
import pprint
import logging
import json
import redis
import os
from sseclient import SSEClient as EventSource
from xml.sax.saxutils import quoteattr

url = 'https://stream.wikimedia.org/v2/stream/recentchange'
rs = redis.Redis('localhost')

# Store pid of this daemon so that we can easily kill it in case we start
# hitting https://phabricator.wikimedia.org/T179986
rs.set("es2r.pid", int(os.getpid()))

def insert_to_redis(wiki, xml):
    result = wiki + "|" + xml
    rs.rpush('rc', result);
    return True;

for event in EventSource(url):
    if event.event == 'message':
        try:
            change = json.loads(event.data)
        except ValueError:
            pass
        else:
            rev_id = ''
            patrolled=False
            length_n=0
            length_o=0
            minor=False
            old = ''
            if ('revision' in change):
                rev_id = 'revid="' + str(change['revision']['new']) + '" '
                if ('old' in change['revision']):
                    old = 'oldid="' + str(change['revision']['old']) + '" '
            if ('patrolled' in change):
                patrolled = change['patrolled']
            if ('minor' in change):
                minor = change['minor']
            if ('length' in change):
                length_n = change['length']['new']
                if ('old' in change['length']):
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
            if (change['type'] == 'log'):
                if ('log_id' in change):
                    result += 'log_id="' + str(change['log_id']) + '" '
                if ('log_type' in change):
                    result += 'log_type=' + quoteattr(change['log_type']) + ' '
                if ('log_action' in change):
                    result += 'log_action=' + quoteattr(change['log_action']) + ' '
                if ('log_action_comment' in change):
                    result += 'log_action_comment=' + quoteattr(change['log_action_comment']) + ' '
            result += 'timestamp="' + str(change['timestamp']) + '">'
            result += '</edit>'
            insert_to_redis(change['server_name'], result)

