import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { WebSocketServer } from 'ws';
import { server, connectToESP32 } from '../src/server.js';

describe('Rotas de status/reconexão da ESP32', () => {
  let mockESP32Server;

  beforeAll(() => {
    process.env.ESP32_WS_URL = 'ws://127.0.0.1:8081';
    mockESP32Server = new WebSocketServer({ port: 8081 });
    connectToESP32();
  });

  afterAll(async () => {
    mockESP32Server.close();
    await new Promise((resolve) => server.close(resolve));
  });

  it('GET /api/esp32/status reflete a ESP32 conectada de verdade', async () => {
    await new Promise((resolve) => mockESP32Server.once('connection', resolve));

    const response = await fetch('http://127.0.0.1:3001/api/esp32/status');
    expect(response.status).toBe(200);

    const body = await response.json();
    expect(body.success).toBe(true);
    expect(body.data.connected).toBe(true);
    expect(body.data.readyState).toBe('OPEN');
    expect(body.data.url).toBe('ws://127.0.0.1:8081');
  });

  it('POST /api/esp32/reconnect força uma nova conexão e o status reflete a reconexão', async () => {
    const novaConexao = new Promise((resolve) => mockESP32Server.once('connection', resolve));

    const reconnectResponse = await fetch('http://127.0.0.1:3001/api/esp32/reconnect', { method: 'POST' });
    expect(reconnectResponse.status).toBe(200);

    const reconnectBody = await reconnectResponse.json();
    expect(reconnectBody.success).toBe(true);

    await novaConexao;

    const statusResponse = await fetch('http://127.0.0.1:3001/api/esp32/status');
    const statusBody = await statusResponse.json();
    expect(statusBody.data.connected).toBe(true);
  });
});
