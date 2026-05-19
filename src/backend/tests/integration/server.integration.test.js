/**
 * Execute:
 *   node --test src/backend/tests/integration/server.integration.test.js
 */

import { describe, it, before, after } from 'node:test';
import assert from 'node:assert/strict';
import { WebSocketServer, WebSocket } from 'ws';

process.env.NODE_ENV        = 'test';
process.env.PORT            = '0';
process.env.HOST            = '127.0.0.1';
process.env.FRONTEND_ORIGIN = '*';

const { server, connectToESP32 } = await import('../../src/server.js');

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function getPort() { return server.address().port; }

async function get(path) {
  const http = (await import('node:http')).default;
  return new Promise((resolve, reject) => {
    http.get(`http://127.0.0.1:${getPort()}${path}`, (res) => {
      let body = '';
      res.on('data', d => (body += d));
      res.on('end', () => {
        try { resolve({ status: res.statusCode, body: JSON.parse(body) }); }
        catch { resolve({ status: res.statusCode, body }); }
      });
    }).on('error', reject);
  });
}

function launchMockESP32() {
  return new Promise((resolve) => {
    const wss = new WebSocketServer({ host: '127.0.0.1', port: 0 });
    wss.on('listening', () => {
      resolve({ wss, url: `ws://127.0.0.1:${wss.address().port}` });
    });
  });
}

function buildMapa(dim) {
  const size = 2 * dim + 1;
  return Array.from({ length: size }, (_, i) =>
    Array.from({ length: size }, (_, j) =>
      i === 0 || i === size - 1 || j === 0 || j === size - 1 ? 1 : 2
    )
  );
}

function buildTelemetry(tipo = '16x16', overrides = {}) {
  const dim = { '4x4': 4, '8x8': 8, '16x16': 16 }[tipo];
  return {
    tipoLabirinto: tipo, bateriaConsumo: '42.5',
    velocidadeMedia: '0.55', tempoConclusao: '5.0',
    desafioCumprido: 'N', mapa: buildMapa(dim), ...overrides,
  };
}

before(async () => {
  if (!server.listening) await new Promise(r => server.once('listening', r));
});

after(async () => {
  await new Promise(r => server.close(r));
});

// ---------------------------------------------------------------------------
// HTTP
// ---------------------------------------------------------------------------

describe('GET /api/health', () => {
  it('retorna 200 com status "online"', async () => {
    const { status, body } = await get('/api/health');
    assert.equal(status, 200);
    assert.equal(body.status, 'online');
    assert.ok(body.timestamp);
  });
});

describe('GET /api/status', () => {
  it('retorna 200 com campos de status', async () => {
    const { status, body } = await get('/api/status');
    assert.equal(status, 200);
    assert.ok(body.frontend);
    assert.ok(body.backend);
  });
});

describe('GET /api/maze/state — sem dados', () => {
  it('retorna 204 ou 200', async () => {
    const { status } = await get('/api/maze/state');
    assert.ok(status === 204 || status === 200);
  });
});

describe('GET /api/maze/history', () => {
  it('retorna 200 com array history', async () => {
    const { status, body } = await get('/api/maze/history');
    assert.equal(status, 200);
    assert.ok(Array.isArray(body.history));
  });
});

describe('Rota inexistente', () => {
  it('retorna 404', async () => {
    const { status } = await get('/api/nao-existe');
    assert.equal(status, 404);
  });
});

// ---------------------------------------------------------------------------
// Pipeline WebSocket ESP32 → backend → frontend
// ---------------------------------------------------------------------------

describe('Pipeline WebSocket completo', () => {
  it('frontend recebe maze_state após ESP32 enviar telemetria', async () => {
    const { wss: mockESP32, url } = await launchMockESP32();
    process.env.ESP32_WS_URL = url;
    const esp32Ws = connectToESP32();
    const frontendWs = new WebSocket(`ws://127.0.0.1:${getPort()}`);

    const msg = await new Promise((resolve, reject) => {
      const t = setTimeout(() => reject(new Error('Timeout')), 4000);
      frontendWs.on('message', raw => { clearTimeout(t); resolve(JSON.parse(raw.toString())); });
      mockESP32.on('connection', c => {
        setTimeout(() => c.send(JSON.stringify(buildTelemetry('16x16'))), 80);
      });
    }).finally(() => { frontendWs.close(); mockESP32.close(); esp32Ws.close?.(); });

    assert.equal(msg.type, 'maze_state');
    const p = msg.payload;
    assert.equal(p.tipoLabirinto, '16x16');
    assert.equal(p.dimension, 16);
    assert.equal(p.totalCells, 256);
    assert.ok(Array.isArray(p.cells));
    assert.ok(Array.isArray(p.rawMatrix));
    assert.ok(p.metrics);
  });

  it('backend ignora JSON inválido da ESP32 sem crash', async () => {
    const { wss: mockESP32, url } = await launchMockESP32();
    process.env.ESP32_WS_URL = url;
    const esp32Ws = connectToESP32();
    const frontendWs = new WebSocket(`ws://127.0.0.1:${getPort()}`);

    const result = await new Promise(resolve => {
      const t = setTimeout(() => resolve('timeout_ok'), 1000);
      frontendWs.on('message', () => { clearTimeout(t); resolve('recebeu_inesperado'); });
      mockESP32.on('connection', c => {
        setTimeout(() => c.send('nao-e-json {{{'), 80);
      });
    }).finally(() => { frontendWs.close(); mockESP32.close(); esp32Ws.close?.(); });

    assert.equal(result, 'timeout_ok');
  });

  it('GET /api/maze/state retorna estado após telemetria processada', async () => {
    const { wss: mockESP32, url } = await launchMockESP32();
    process.env.ESP32_WS_URL = url;
    const esp32Ws = connectToESP32();

    await new Promise((resolve, reject) => {
      const t = setTimeout(() => reject(new Error('Timeout')), 4000);
      mockESP32.on('connection', c => {
        setTimeout(async () => {
          c.send(JSON.stringify(buildTelemetry('8x8')));
          await new Promise(r => setTimeout(r, 200));
          clearTimeout(t); resolve();
        }, 80);
      });
    }).finally(() => { mockESP32.close(); esp32Ws.close?.(); });

    const { status, body } = await get('/api/maze/state');
    assert.equal(status, 200);
    assert.ok(body.tipoLabirinto);
    assert.ok(body.dimension);
  });
});

// ---------------------------------------------------------------------------
// CA-08 via snapshot HTTP
// ---------------------------------------------------------------------------

describe('CA-08 via snapshot HTTP', () => {
  it('CA-08.1 — cells tem walls com n/s/l/o', async () => {
    const { status, body } = await get('/api/maze/state');
    if (status === 204) return;
    const c = body.cells[0][0];
    assert.ok('north' in c.walls && 'south' in c.walls && 'east' in c.walls && 'west' in c.walls);
  });

  it('CA-08.2 — cells tem campo visited booleano', async () => {
    const { status, body } = await get('/api/maze/state');
    if (status === 204) return;
    body.cells.flat().forEach(c => assert.equal(typeof c.visited, 'boolean'));
  });

  it('CA-08.4 — cells tem exits e exitCount', async () => {
    const { status, body } = await get('/api/maze/state');
    if (status === 204) return;
    const c = body.cells[0][0];
    assert.ok('exits' in c);
    assert.equal(typeof c.exitCount, 'number');
  });
});