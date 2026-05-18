import { createServer } from "node:http";
import { WebSocketServer, WebSocket } from "ws";

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
      backend: "Node.js ativo (HTTP + WebSockets)",
      database: "pendente de definicao",
      sensors: "pendente de definicao"
    });
    return;
  }

  sendJson(response, 404, {
    error: "Rota nao encontrada"
  });
});

const wssReact = new WebSocketServer({ server });

wssReact.on("connection", (ws) => {
  console.log("[backend] Frontend React conectado ao WebSocket.");
  ws.on("close", () => console.log("[backend] Frontend React desconectado."));
});

function connectToESP32() {
  const esp32Url = process.env.ESP32_WS_URL || "ws://192.168.4.1:81";
  console.log(`[backend] Tentando conectar na ESP32 em ${esp32Url}...`);
  const esp32Ws = new WebSocket(esp32Url);

  esp32Ws.on("open", () => {
    console.log("[backend] Conectado com sucesso à ESP32-C3!");
  });

  esp32Ws.on("message", (data) => {
    wssReact.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(data.toString());
      }
    });
  });

  esp32Ws.on("close", () => {
    console.log("[backend] Conexão com a ESP32 perdida. Tentando reconectar em 2 segundos...");
    setTimeout(connectToESP32, 2000);
  });

  esp32Ws.on("error", (err) => {
    console.error("[backend] Erro na conexão com a ESP32:", err.message);
  });
}

if (process.env.NODE_ENV !== 'test') {
  connectToESP32();
}

server.listen(port, host, () => {
  console.log(`[backend] API e WebSocket escutando em http://${host}:${port}`);
});

export { server, wssReact, connectToESP32 };