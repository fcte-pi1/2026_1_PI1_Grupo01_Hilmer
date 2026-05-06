import { createServer } from "node:http";

const port = Number(process.env.PORT || 3001);
const host = process.env.HOST || "127.0.0.1";
const frontendOrigin = process.env.FRONTEND_ORIGIN || "http://localhost:5173";

function sendJson(response, statusCode, payload) {
  response.writeHead(statusCode, {
    "Content-Type": "application/json; charset=utf-8",
    "Access-Control-Allow-Origin": frontendOrigin,
    "Access-Control-Allow-Methods": "GET,OPTIONS",
    "Access-Control-Allow-Headers": "Content-Type"
  });

  response.end(JSON.stringify(payload));
}

const server = createServer((request, response) => {
  const url = new URL(request.url || "/", `http://${request.headers.host}`);

  if (request.method === "OPTIONS") {
    response.writeHead(204, {
      "Access-Control-Allow-Origin": frontendOrigin,
      "Access-Control-Allow-Methods": "GET,OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type"
    });
    response.end();
    return;
  }

  if (request.method === "GET" && url.pathname === "/api/health") {
    sendJson(response, 200, {
      status: "online",
      message: "API base do projeto disponível.",
      runtime: "node-http",
      pending: {
        database: "nao_definido",
        sensors: "nao_definido"
      },
      timestamp: new Date().toISOString()
    });
    return;
  }

  if (request.method === "GET" && url.pathname === "/api/status") {
    sendJson(response, 200, {
      frontend: "esperado em React + Vite",
      backend: "Node.js ativo",
      database: "pendente de definicao",
      sensors: "pendente de definicao"
    });
    return;
  }

  sendJson(response, 404, {
    error: "Rota nao encontrada"
  });
});

server.listen(port, host, () => {
  console.log(`[backend] API escutando em http://${host}:${port}`);
});
