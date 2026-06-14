import { createServer } from 'node:http';
import express from 'express';
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import {
  historico4x4,
  telemetria4x4,
  trajeto4x4,
} from '../fixtures/analysisAttempt.js';

const mockService = vi.hoisted(() => ({
  buscarHistoricoPorTentativa: vi.fn(),
  listarTrajetoPorTentativa: vi.fn(),
  listarTelemetriaPorTentativa: vi.fn(),
}));

vi.mock('../../src/services/simulationService.js', () => ({
  default: mockService,
}));

const simulationRoutes = await import('../../src/routes/simulationRoutes.js');

let server;
let baseUrl;

beforeEach(async () => {
  mockService.buscarHistoricoPorTentativa.mockReset();
  mockService.listarTrajetoPorTentativa.mockReset();
  mockService.listarTelemetriaPorTentativa.mockReset();

  const app = express();
  app.use(express.json());
  app.use('/api', simulationRoutes.default);

  server = createServer(app);
  await new Promise((resolve) => server.listen(0, '127.0.0.1', resolve));
  const { port } = server.address();
  baseUrl = `http://127.0.0.1:${port}`;
});

afterEach(async () => {
  if (server) {
    await new Promise((resolve) => server.close(resolve));
  }
});

describe('GET /api/historico/:numTentativa/analise', () => {
  it('retorna 404 quando a tentativa não existe', async () => {
    mockService.buscarHistoricoPorTentativa.mockResolvedValueOnce(null);

    const response = await fetch(`${baseUrl}/api/historico/999/analise`);
    const body = await response.json();

    expect(response.status).toBe(404);
    expect(body.success).toBe(false);
    expect(body.error).toMatch(/não encontrada/i);
  });

  it('retorna 404 quando não há trajeto', async () => {
    mockService.buscarHistoricoPorTentativa.mockResolvedValueOnce(historico4x4);
    mockService.listarTrajetoPorTentativa.mockResolvedValueOnce([]);

    const response = await fetch(`${baseUrl}/api/historico/1/analise`);
    const body = await response.json();

    expect(response.status).toBe(404);
    expect(body.success).toBe(false);
    expect(body.error).toMatch(/trajeto/i);
  });

  it('retorna análise com caminhos e grid quando há dados', async () => {
    mockService.buscarHistoricoPorTentativa.mockResolvedValueOnce({ tipolabirinto: '8x8' });
    mockService.listarTrajetoPorTentativa.mockResolvedValueOnce(trajeto4x4);
    mockService.listarTelemetriaPorTentativa.mockResolvedValueOnce(telemetria4x4);

    const response = await fetch(`${baseUrl}/api/historico/1/analise`);
    const body = await response.json();

    expect(response.status).toBe(200);
    expect(body.success).toBe(true);
    expect(body.data.outboundPath.length).toBeGreaterThan(0);
    expect(body.data.optimalPath.length).toBeGreaterThan(0);
    expect(Array.isArray(body.data.grid)).toBe(true);
    expect(body.data.grid.flat().some((cell) => cell === 2)).toBe(true);
  });
});
