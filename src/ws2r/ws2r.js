// npm install socket.io-client@0.9.1

var io = require( 'socket.io-client' );
var socket = io.connect( 'stream.wikimedia.org/rc' );
var redis = require('redis');
var client = redis.createClient(); 

function insert2redis(wiki, xml) {
    client.rpush("rc", wiki + "|" + xml);
    return true;
}

function quoteattr(xml) {
    var escaped = xml.replace('"', '&quot;').replace("'", '&apos;').replace("\n", " ").replace("<", "&lt;").replace(">", "&gt;");
    return '"' + escaped + '"';
}
  
socket.on( 'connect', function () {
     socket.emit( 'subscribe', '*' );
} );
 
socket.on( 'change', function ( data ) {
     var xml = '';
     var rev_id = '';
     var patrolled='false';
     var length_n=0;
     var length_o=0;
     var minor='false';
     var old = '';
     if (data.revision !== undefined) {
         if (data.revision.new !== null) {
             rev_id = 'revid=' + quoteattr(data.revision.new.toString()) + ' ';
         }
         if (data.revision.old !== null) {
             old = 'oldid=' + quoteattr(data.revision.old.toString()) + ' ';
         }
     }
     if (data.patrolled !== undefined) {
         patrolled = data.patrolled.toString();
     }
     if (data.minor !== undefined) {
         minor = data.minor.toString();
     }
     if (data.length !== undefined) {
         if (data.length.new !== null) {
             length_n = data.length.new;
         }
         if (data.length.old != null) {
             length_o = data.length.old;
         }
     }
     xml += '<edit wiki=' + quoteattr(data.wiki) + ' ';
     xml += 'server_name=' + quoteattr(data.server_name) + ' ';
     xml += 'summary=' + quoteattr(data.comment) + ' ';
     xml += rev_id + old;
     xml += 'title=' +  quoteattr(data.title) + ' ';
     xml += 'namespace=' + quoteattr(data.namespace.toString()) + ' ';
     xml += 'user=' + quoteattr(data.user) + ' ';
     xml += 'bot=' + quoteattr(data.bot.toString()) + ' ';
     xml += 'patrolled=' + quoteattr(patrolled) + ' ';
     xml += 'minor=' + quoteattr(minor) + ' ';
     xml += 'type=' + quoteattr(data.type) + ' ';
     xml += 'length_new=' + quoteattr(length_n.toString()) + ' ';
     xml += 'length_old=' + quoteattr(length_o.toString()) + ' ';
     xml += 'timestamp=' + quoteattr(data.timestamp.toString()) + '>';
     xml += '</edit>';
     insert2redis(data.server_name, xml);
} );
