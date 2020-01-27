const cluster = require('cluster');
const numCPUs = require('os').cpus().length;
const Koa = require('koa');
const route = require('koa-route');
const app = new Koa();

if (cluster.isMaster) {
  console.log(`Master ${process.pid} is running`);

  // Fork workers.
  for (let i = 0; i < numCPUs; i++) {
    cluster.fork();
  }

} else {

  app.use(route.get('/', ctx => {
    ctx.response.body = 'hello,world\n';
  }));
  // app.use(async ctx => {
  //   ctx.body = 'hello,world\n';
  // });

  app.listen(3000);

}