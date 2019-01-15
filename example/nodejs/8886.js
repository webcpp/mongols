var net = require('net');

// Configuration parameters
var HOST = 'localhost';
var PORT = 8886;

// Create Server instance 
var server = net.createServer(onClientConnected);

server.listen(PORT, HOST, function () {
    console.log('server listening on %j', server.address());
});

function onClientConnected(sock) {

    sock.on('data', function (data) {
        console.log(data.toString());
        sock.write(data);
    });

}
;
