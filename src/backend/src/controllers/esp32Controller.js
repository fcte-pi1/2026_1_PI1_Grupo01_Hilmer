/**
 * Gerencia a conexão WebSocket com a ESP32 (ou simulador).
 * Responsabilidade única: receber mensagens brutas, fazer parse JSON
 * e delegar o processamento para o mazeController.
 */

import { WebSocket } from 'ws';
import { handleTelemetryUpdate } from './mazeController.js';

const RECONNECT_DELAY_MS = 2000;

/**
 * Conecta ao endpoint WebSocket da ESP32 e mantém reconexão automática.
 *
 * @returns {WebSocket} — instância exposta para testes
 */
export function connectToESP32() {
  const esp32Url = process.env.ESP32_WS_URL || 'ws://192.168.4.1:81';
  console.log(`[esp32] Tentando conectar em ${esp32Url}...`);

  const ws = new WebSocket(esp32Url);

  ws.on('open', () => {
    console.log('[esp32] Conectado com sucesso à ESP32-C3!');
  });

  ws.on('message', (data) => {
    let raw;
    try {
      raw = JSON.parse(data.toString());
    } catch {
      console.warn('[esp32] Mensagem não-JSON recebida, ignorada.');
      return;
    }

    try {
      handleTelemetryUpdate(raw);
    } catch (err) {
      console.error('[esp32] Erro ao processar telemetria:', err.message);
    }
  });

  ws.on('close', () => {
    console.log(`[esp32] Conexão perdida. Reconectando em ${RECONNECT_DELAY_MS / 1000}s...`);
    setTimeout(connectToESP32, RECONNECT_DELAY_MS);
  });

  ws.on('error', (err) => {
    console.error('[esp32] Erro de conexão:', err.message);
  });

  return ws;
}