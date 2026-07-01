/**
 * server.js — Servidor principal do Micromouse
 *
 * Express (HTTP + rotas) + WebSocket (ESP32 → Frontend)
 */

import express from 'express';
import cors from 'cors';
import { createServer } from 'node:http';
import { WebSocketServer, WebSocket } from 'ws';
import dotenv from 'dotenv';
import mouseRoutes from './routes/mouseRoutes.js';
import simulationRoutes from './routes/simulationRoutes.js';
import liveTelemetryBuffer from './services/liveTelemetryBuffer.js';

dotenv.config();

const port = Number(process.env.PORT || 3001);
const host = process.env.HOST || '127.0.0.1';
const frontendOrigin = process.env.FRONTEND_ORIGIN || 'http://localhost:5173';

const app = express();

app.use(cors({ origin: frontendOrigin, methods: ['GET', 'POST', 'OPTIONS'] }));
app.use(express.json());

app.get('/api/health', (_req, res) => {
  res.json({ status: 'online', timestamp: new Date().toISOString() });
});

app.get('/api/status', (_req, res) => {
  res.json({
    frontend: frontendOrigin,
    backend: `http://${host}:${port}`,
    database: {
      host: process.env.DB_HOST || 'localhost',
      port: Number(process.env.DB_PORT || 5432),
      name: process.env.DB_NAME || 'micromouse_db',
    },
  });
});

app.use('/api/mouse', mouseRoutes);
app.use('/api', simulationRoutes);

app.use((err, _req, res, _next) => {
  console.error(`[ERROR] ${err.message}`);
  res.status(500).json({ success: false, error: 'Erro interno no servidor.' });
});

const httpServer = createServer(app);
const wssReact = new WebSocketServer({ server: httpServer });

wssReact.on('connection', (ws) => {
  console.log('[backend] Frontend React conectado via WebSocket.');

  // Repassa comandos vindos do site (ex: "INICIAR_CORRIDA") direto para a
  // ESP32 conectada. Sem isso, o botão "Iniciar corrida" do dashboard não
  // tem nenhum efeito no robô.
  ws.on('message', (data) => {
    if (esp32Socket && esp32Socket.readyState === WebSocket.OPEN) {
      esp32Socket.send(data.toString());
    } else {
      console.warn('[backend] Comando do site descartado: ESP32 não está conectada.');
    }
  });

  ws.on('close', () => console.log('[backend] Frontend React desconectado.'));
});

wssReact.on('error', (err) => {
  console.error('[backend] Erro no WebSocket server:', err.message);
});

function broadcastToFrontend(data) {
  const payload = data.toString();
  wssReact.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(payload);
    }
  });
}

let startPromise = null;
let esp32Socket = null;
let reconnectTimeout = null;

export function ensureServerStarted() {
  if (httpServer.listening) {
    return Promise.resolve();
  }

  if (startPromise) {
    return startPromise;
  }

  startPromise = new Promise((resolve, reject) => {
    httpServer.listen(port, host, () => {
      console.log(`[backend] Servidor rodando em http://${host}:${port}`);
      resolve();
    });

    httpServer.once('error', (err) => {
      startPromise = null;
      reject(err);
    });
  });

  return startPromise;
}

function scheduleReconnect() {
  clearTimeout(reconnectTimeout);
  reconnectTimeout = setTimeout(() => {
    connectToESP32();
  }, 2000);
}

export function connectToESP32() {
  const esp32Url = process.env.ESP32_WS_URL || 'ws://192.168.4.1:81';

  if (
    esp32Socket &&
    (esp32Socket.readyState === WebSocket.OPEN || esp32Socket.readyState === WebSocket.CONNECTING)
  ) {
    return esp32Socket;
  }

  console.log(`[backend] Tentando conectar à ESP32 em ${esp32Url}...`);
  esp32Socket = new WebSocket(esp32Url);

  esp32Socket.on('open', () => {
    console.log('[backend] Conectado à ESP32-C3!');
  });

  esp32Socket.on('message', (data) => {
    try {
      const payload = JSON.parse(data.toString());

      // A ESP32 não tem o numTentativa real do banco (só existe quando o
      // site persiste o HISTORICO ao final da corrida), então os snapshots
      // ficam num buffer em memória e só são gravados quando esse
      // HISTORICO é criado (ver POST /api/historico em simulationRoutes.js).
      if (payload?.status === 'waiting') {
        liveTelemetryBuffer.limpar();
      } else if (payload?.tempoColeta) {
        liveTelemetryBuffer.registrar(payload);
      }

      broadcastToFrontend(data);
    } catch (err) {
      console.error('[backend] Erro ao processar mensagem da ESP32:', err.message);
    }
  });

  esp32Socket.on('close', () => {
    console.log('[backend] ESP32 desconectada. Reconectando em 2s...');
    esp32Socket = null;
    scheduleReconnect();
  });

  esp32Socket.on('error', (err) => {
    console.error('[backend] Erro ESP32:', err.message);
  });

  return esp32Socket;
}

await ensureServerStarted();

if (process.env.NODE_ENV !== 'test') {
  connectToESP32();
}

export { app, httpServer, httpServer as server, wssReact };
