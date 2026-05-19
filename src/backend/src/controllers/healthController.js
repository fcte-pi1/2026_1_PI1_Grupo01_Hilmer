export function getHealth(req, res, sendJson) {
    sendJson(res, 200, {
      status:    'online',
      message:   'API base do projeto disponível.',
      runtime:   'node-http',
      timestamp: new Date().toISOString(),
    });
  }
 
  export function getStatus(req, res, sendJson) {
    sendJson(res, 200, {
      frontend: 'esperado em React + Vite',
      backend:  'Node.js ativo (HTTP + WebSockets)',
      database: 'pendente de definicao',
      sensors:  'pendente de definicao',
    });
  }