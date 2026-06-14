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
import simulationService from './services/simulationService.js';

dotenv.config();

const port = Number(process.env.PORT || 3001);
const host = process.env.HOST || '127.0.0.1';
const frontendOrigin = process.env.FRONTEND_ORIGIN || 'http://localhost:5173';

const app = express();

app.use(cors({ origin: frontendOrigin, methods: ['GET', 'POST', 'OPTIONS'] }));
app.use(express.json());

// ─── Rotas ───────────────────────────────────────────────────────────────────

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
app.use('/api', simulationRoutes); // monta: /api/historico, /api/telemetria, /api/trajeto

app.use((err, _req, res, _next) => {
  console.error(`[ERROR] ${err.message}`);
  res.status(500).json({ success: false, error: 'Erro interno no servidor.' });
});

// ─── HTTP Server + WebSocket ──────────────────────────────────────────────────

const httpServer = createServer(app);
const wssReact = new WebSocketServer({ server: httpServer });

wssReact.on('connection', (ws) => {
  console.log('[backend] Frontend React conectado via WebSocket.');
  ws.on('close', () => console.log('[backend] Frontend React desconectado.'));
});

function broadcastToFrontend(data) {
  wssReact.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) client.send(data.toString());
  });
}

// ─── Conexão com ESP32 ────────────────────────────────────────────────────────

/**
 * A ESP32 envia JSON com os campos de TELEMETRIA.
 * O backend persiste no banco E retransmite ao frontend simultaneamente.
 *
 * Payload esperado da ESP32:
 * {
 *   numTentativa, tempoColeta, tensaoRecente, correnteRecente,
 *   posHRecente, posVRecente, velocidadeAtual, bateriaAtual, tensaoAtual,
 *   sensorCor?, sensorEsquerda?, sensorDireita?, sensorFrontal?
 * }
 */
function connectToESP32() {
  const esp32Url = process.env.ESP32_WS_URL || 'ws://192.168.4.1:81';
  console.log(`[backend] Tentando conectar à ESP32 em ${esp32Url}...`);

  const esp32Ws = new WebSocket(esp32Url);

  esp32Ws.on('open', () => console.log('[backend] Conectado à ESP32-C3!'));

  esp32Ws.on('message', async (data) => {
    try {
      const payload = JSON.parse(data.toString());

      // Persiste no banco
      // await simulationService.inserirTelemetria(payload);

      // Retransmite ao frontend
      broadcastToFrontend(data);
    } catch (err) {
      // TODO: separar erro de parse de erro de banco para log mais preciso
      console.error('[backend] Erro ao processar mensagem da ESP32:', err.message);
    }
  });

  esp32Ws.on('close', () => {
    console.log('[backend] ESP32 desconectada. Reconectando em 2s...');
    setTimeout(connectToESP32, 2000);
  });

  esp32Ws.on('error', (err) => {
    console.error('[backend] Erro ESP32:', err.message);
  });
}

// ─── Inicialização ────────────────────────────────────────────────────────────

let startedByTests = false;

export function ensureServerStarted() {
  if (httpServer.listening) {
    return Promise.resolve();
  }

  return new Promise((resolve, reject) => {
    const onError = (err) => {
      if (err.code === 'EADDRINUSE') {
        resolve();
        return;
      }
      reject(err);
    };

    httpServer.once('error', onError);
    httpServer.listen(port, host, () => {
      httpServer.off('error', onError);
      startedByTests = true;
      console.log(`[backend] Servidor rodando em http://${host}:${port}`);
      resolve();
    });
  });
}

export function stopServerIfStarted() {
  if (!startedByTests || !httpServer.listening) {
    return Promise.resolve();
  }

  return new Promise((resolve) => {
    httpServer.close(() => {
      startedByTests = false;
      resolve();
    });
  });
}

function startHttpServer() {
  const onListenError = (err) => {
    if (err.code === 'EADDRINUSE') {
      console.error(
        `[backend] Porta ${port} já está em uso. Encerre o processo anterior (ex.: netstat -ano | findstr :${port}) e tente novamente.`,
      );
    } else {
      console.error('[backend] Erro ao iniciar servidor:', err.message);
    }
    process.exit(1);
  };

  httpServer.once('error', onListenError);
  wssReact.once('error', onListenError);

  httpServer.listen(port, host, () => {
    console.log(`[backend] Servidor rodando em http://${host}:${port}`);
  });
}

if (process.env.NODE_ENV !== 'test') {
  connectToESP32();
  startHttpServer();
}

export { app, httpServer, httpServer as server, wssReact, connectToESP32 };