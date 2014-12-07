/*
  Prerequisites:

    1. Install node.js and npm
    2. npm install ws

  See also,

    http://einaros.github.com/ws/

  To run,

    node example-server.js
*/

"use strict"; // http://ejohn.org/blog/ecmascript-5-strict-mode-json-and-more/
var WebSocketServer = require('ws').Server;
var http = require('http');
var app = http.createServer();
var server; // assigned by app.listen() below

var wssEchoWithSize = new WebSocketServer({server: app, path: '/echoWithSize'});
wssEchoWithSize.on('connection', function(ws) {
    ws.on('message', function(data, flags) {
        if (flags.binary) { return; }
        ws.send(data.length + '\n' + data);
    });
    ws.on('close', function() {
    });
    ws.on('error', function(e) {
    });
});

var wssBinaryEchoWithSize = new WebSocketServer({server: app, path: '/binaryEchoWithSize'});
wssBinaryEchoWithSize.on('connection', function(ws) {
    ws.on('message', function(data, flags) {
        if (!flags.binary) { return; }
        //var result = new ArrayBuffer(data.length + 4);
        //new DataView(result).setInt32(0, data.length, false); // false = big endian
        var result = new Buffer(data.length + 4);
        result.writeInt32BE(data.length, 0);
        data.copy(result, 4, 0, data.length);
        ws.send(result, { binary: true });
    });
    ws.on('close', function() {
    });
    ws.on('error', function(e) {
    });
});

var wssKillServer = new WebSocketServer({server: app, path: '/killServer'});
wssKillServer.on('connection', function(ws) {
    ws.on('message', function(data, flags) {
        if (flags.binary) { return; }
        server.close();
    });
    ws.on('close', function() {
    });
    ws.on('error', function(e) {
    });
});

var PORT = parseInt(process.env.PORT || '8123');
server = app.listen(PORT);
console.log('Listening on port ' + PORT + '...');
