import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { WebSocketServer, WebSocket } from 'ws';
import { server, wssReact, connectToESP32 } from '../src/server.js';

describe('Teste de Integração: Broker WebSocket', () => {
  let mockESP32Server;
  let clienteReactFalso;
  let socketBackendESP32;

  beforeAll(() => {
    process.env.ESP32_WS_URL = 'ws://127.0.0.1:8080';
    mockESP32Server = new WebSocketServer({ port: 8080 });

    connectToESP32();
  });

  afterAll(() => {
    mockESP32Server.close();
    if (clienteReactFalso) clienteReactFalso.close();
    server.close();
    wssReact.close();
  });

  it('O Backend deve repassar a telemetria da ESP32 para o Frontend', async () => {
    const telemetriaFalsa = {
      numTentativa: 42,
      tempoColeta: new Date().toISOString(),
      tensaoRecente: 7.1,
      correnteRecente: 1.2,
      posHRecente: 3,
      posVRecente: 5,
      velocidadeAtual: 0.6,
      bateriaAtual: 84.5,
      tensaoAtual: 7.3,
      sensorCor: '#ffffff',
      sensorEsquerda: 120,
      sensorDireita: 140,
      sensorFrontal: 110,
      tipoLabirinto: '16x16',
    };

    const backendConectouNaESP32 = new Promise((resolve) => {
      mockESP32Server.once('connection', (socket) => {
        socketBackendESP32 = socket;
        resolve();
      });
    });

    const frontendConectou = new Promise((resolve, reject) => {
      clienteReactFalso = new WebSocket('ws://127.0.0.1:3001');
      clienteReactFalso.once('open', resolve);
      clienteReactFalso.once('error', reject);
    });

    await Promise.all([backendConectouNaESP32, frontendConectou]);

    return new Promise((resolve, reject) => {
      const timeout = setTimeout(() => reject(new Error('Timeout: O Backend não repassou a mensagem')), 3000);

      clienteReactFalso.once('message', (data) => {
        try {
          const recebido = JSON.parse(data);
          expect(recebido.numTentativa).toBe(42);
          expect(recebido.sensorCor).toBe('#ffffff');
          expect(recebido.tensaoAtual).toBe(7.3);
          clearTimeout(timeout);
          resolve();
        } catch (erro) {
          clearTimeout(timeout);
          reject(erro);
        }
      });

      socketBackendESP32.send(JSON.stringify(telemetriaFalsa));
    });
  });
});