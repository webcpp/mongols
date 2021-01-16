const cluster = require('cluster');
const numCPUs = require('os').cpus().length;
// Require the framework and instantiate it
const fastify = require('fastify')({ logger: false })



if (cluster.isMaster) {
  console.log(`Master ${process.pid} is running`);

  // Fork workers.
  for (let i = 0; i < numCPUs; i++) {
    cluster.fork();
  }

} else {

  // Declare a route
  fastify.get('/', async (request, reply) => {
    reply.header('Content-Type', 'text/plain;charset=utf-8')
    return 'hello,world\n';
  })

  // Run the server!
  const start = async () => {
    try {
      await fastify.listen(3000)
      fastify.log.info(`server listening on ${fastify.server.address().port}`)
    } catch (err) {
      fastify.log.error(err)
    }
  }
  start()

}
