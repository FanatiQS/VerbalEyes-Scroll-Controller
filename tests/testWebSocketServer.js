const fs = require('fs');
const http = require('http');
const WebSocket = require('../../teleprompter-server/node_modules/ws/index.js');

const server = http.createServer((req, res) => {
	res.end(fs.readFileSync('./index.html'));
});

const wss = new WebSocket.Server({ server: server, clientTracking: true });
wss.on('connection', function connection(ws) {
	console.log('new connection');
	ws.on('message', function (message) {
		try {
			let output;
			const data = JSON.parse(message)._core;
			if (data.auth && data.auth.hasOwnProperty("id")) {
				ws.send('{"authed": data}');
				console.log('client connected to project:', data.id);
				return;
			}
			else if (data.doc && data.doc.hasOwnProperty('speed')) {
				output = 'speed was updated to: ' + data.doc.speed;
			} else if (data.doc && data.doc.hasOwnProperty('offset')) {
				output = 'position was updated to: ' + data.doc.offset;
			}

			console.log(output);
			for (const client of wss.clients) {
				if (client !== this) client.send(output + '\n');
			}
		}
		catch(err){
			console.log('failed to parse\n', err, '\n', message);
		}
	});
	ws.on('close', function (message) {
		console.log('close:', message);
	});
	ws.on('error', function (message) {
		console.log('error:', message);
	});
});

server.listen(8080);
