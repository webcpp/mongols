var http = require('http');
var port =8889;

http.createServer(function (request, response) {

    response.writeHead(200, {'Content-Type': 'text/plain'});

    response.end('Hello World\n');
}).listen(port);


console.log('Server running at http://127.0.0.1:'+8889+'/');
