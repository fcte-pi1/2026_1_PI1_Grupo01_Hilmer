import { afterEach, describe, expect, it, vi } from 'vitest';
import attemptService from '../src/services/attemptService.js';
import liveTelemetryBuffer from '../src/services/liveTelemetryBuffer.js';
import liveTrajetoBuffer from '../src/services/liveTrajetoBuffer.js';

const mockCriarHistorico = vi.fn();
const mockInserirTelemetriaEmLote = vi.fn();
const mockInserirTrajetoEmLote = vi.fn();

vi.mock('../src/services/simulationService.js', () => ({
  default: {
    criarHistorico: (...args) => mockCriarHistorico(...args),
    inserirTelemetriaEmLote: (...args) => mockInserirTelemetriaEmLote(...args),
    inserirTrajetoEmLote: (...args) => mockInserirTrajetoEmLote(...args),
  },
}));

afterEach(() => {
  attemptService.resetAttemptState();
  mockCriarHistorico.mockReset();
  mockInserirTelemetriaEmLote.mockReset();
  mockInserirTrajetoEmLote.mockReset();
});

describe('attemptService', () => {
  it('limpa buffers em waiting_start', async () => {
    liveTelemetryBuffer.registrar({ tempoColeta: '2026-01-01T10:00:00.000Z' });
    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });

    await attemptService.handleLiveTelemetry({ status: 'waiting_start' });

    expect(liveTelemetryBuffer.drenar()).toEqual([]);
    expect(liveTrajetoBuffer.listar()).toEqual([]);
  });

  it('persiste tentativa em success de forma idempotente', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 42 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    liveTelemetryBuffer.registrar({
      tempoColeta: '2026-01-01T10:00:00.000Z',
      posHRecente: 1,
      posVRecente: 0,
    });

    const payload = {
      status: 'success',
      numTentativa: 9001,
      mazeSize: 8,
      tipoLabirinto: '8x8',
      batteryPercent: 80,
      speedMps: 0.5,
      tempoColeta: '2026-01-01T10:00:01.000Z',
      trajetoAtual: { passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' },
      visitedPath: [[1, 1], [2, 1]],
    };

    const first = await attemptService.handleLiveTelemetry(payload);
    const second = await attemptService.handleLiveTelemetry(payload);

    expect(first.alreadyPersisted).toBe(false);
    expect(second.alreadyPersisted).toBe(true);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(1);
    expect(mockInserirTrajetoEmLote).toHaveBeenCalled();
  });
});
