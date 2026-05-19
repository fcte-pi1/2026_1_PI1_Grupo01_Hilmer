import { createServer } from 'node:http';
import { routes }               from './routes/apiRoutes.js';
import { initReactBroadcaster } from './ws/reactBroadcaster.js';
import { connectToESP32 }       from './controllers/esp32Controller.js';
import { getLatestMazeState }   from './controllers/mazeController.js';

const port           = Number(process.env.PORT           || 3001);
const host           = process.env.HOST                  || '127.0.0.1';
const frontendOrigin = process.env.FRONTEND_ORIGIN       || 'http://localhost:5173';

function sendJson(res, statusCode, payload) {
  res.writeHead(statusCode, {
    'Content-Type':                'application/json; charset=utf-8',
    'Access-Control-Allow-Origin':  frontendOrigin,
    'Access-Control-Allow-Methods': 'GET,OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
  });
  res.end(JSON.stringify(payload));
}

export const server = createServer((req, res) => {
  const url = new URL(req.url || '/', `http://${req.headers.host}`);

  // Pre-flight CORS
  if (req.method === 'OPTIONS') {
    res.writeHead(204, {
      'Access-Control-Allow-Origin':  frontendOrigin,
      'Access-Control-Allow-Methods': 'GET,OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type',
    });
    res.end();
    return;
  }

  // Despacha para o handler correto
  const route = routes.find(r => r.method === req.method && r.path === url.pathname);
  if (route) {
    route.handler(req, res, sendJson);
    return;
  }

  sendJson(res, 404, { error: 'Rota nao encontrada' });
});


const wssReact = initReactBroadcaster(server, getLatestMazeState);
export { wssReact };

if (process.env.NODE_ENV !== 'test') {
  connectToESP32();
}

server.listen(port, host, () => {
  console.log(`[backend] Escutando em http://${host}:${port}`);
});

export { connectToESP32 };