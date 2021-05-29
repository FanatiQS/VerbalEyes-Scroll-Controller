const http = require('http');
const {createHash} = require('crypto');

http.createServer().on('upgrade', function (req, socket) {
	socket.write("HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Accept: " + createHash('sha1').update(req.headers['sec-websocket-key'] + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").digest('base64'));
	socket.on('data', function (chunk) {
		console.log(chunk.toString());
		socket.write(String.fromCharCode(129, 9) + "\"authed\":", 'binary');
	});
}).listen(1994);
