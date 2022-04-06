const { createProxyMiddleware } = require('http-proxy-middleware');

module.exports = function(app) {
  app.use(
    '/api',
    createProxyMiddleware({
      target: 'https://89.58.41.196:11800',
      changeOrigin: true,
      ignorePath: false,
      secure: false
    })
  );
  
  app.use(
    createProxyMiddleware('/api/ws/frontend', {
        target: 'wss://89.58.41.196:11800',
        ws: true,
        changeOrigin: true,
        secure: false
    })
);
};