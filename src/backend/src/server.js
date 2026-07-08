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
import createEsp32Routes from './routes/esp32Routes.js';
import attemptService from './services/attemptService.js';

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
    const raw = data.toString();

    try {
      const command = JSON.parse(raw);
      if (command?.type === 'START' || command?.type === 'START_RUN') {
        attemptService.resetAttemptState();
      }
    } catch {
      // Comando não-JSON: repassa à ESP32 sem alterar estado.
    }

    if (esp32Socket && esp32Socket.readyState === WebSocket.OPEN) {
      esp32Socket.send(raw);
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

function getESP32Url() {
  return process.env.ESP32_WS_URL || 'ws://192.168.4.1:81';
}

const READY_STATE_NAMES = ['CONNECTING', 'OPEN', 'CLOSING', 'CLOSED'];

export function connectToESP32() {
  const esp32Url = getESP32Url();

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

      attemptService.handleLiveTelemetry(payload).catch((err) => {
        console.error('[backend] Falha ao persistir tentativa ao vivo:', err.message);
      });

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

export function getESP32Status() {
  return {
    connected: !!esp32Socket && esp32Socket.readyState === WebSocket.OPEN,
    url: getESP32Url(),
    readyState: esp32Socket ? READY_STATE_NAMES[esp32Socket.readyState] : 'CLOSED',
  };
}

// Fecha qualquer socket existente (aberto, conectando ou travado numa
// leitura ruim) e tenta uma conexão nova imediatamente, ignorando o
// cronômetro de reconexão automática — usado pelo botão manual do site.
export function forceReconnectESP32() {
  clearTimeout(reconnectTimeout);

  if (esp32Socket) {
    esp32Socket.removeAllListeners();
    esp32Socket.terminate();
    esp32Socket = null;
  }

  return connectToESP32();
}

// Montada aqui embaixo (não junto com as outras rotas, lá em cima) porque
// depende de getESP32Status/forceReconnectESP32, definidas só agora — a
// ordem de app.use() não importa pro Express, só precisa vir antes do
// ensureServerStarted() logo abaixo.
app.use('/api/esp32', createEsp32Routes({ getStatus: getESP32Status, reconnect: forceReconnectESP32 }));

await ensureServerStarted();

if (process.env.NODE_ENV !== 'test') {
  connectToESP32();
}

export { app, httpServer, httpServer as server, wssReact };
