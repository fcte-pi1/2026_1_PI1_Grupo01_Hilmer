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

const baseSuccessPayload = {
  status: 'success',
  numTentativa: 9001,
  mazeSize: 8,
  tipoLabirinto: '8x8',
  batteryPercent: 80,
  speedMps: 0.5,
  tempoColeta: '2026-01-01T10:00:01.000Z',
  trajetoAtual: { passo: 2, pos_h: 1, pos_v: 0, direcao: 'LESTE' },
  visitedPath: [[1, 1], [2, 1]],
};

describe('attemptService', () => {
  it('limpa buffers e idempotência em waiting_start', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 42 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    await attemptService.handleLiveTelemetry(baseSuccessPayload);

    liveTelemetryBuffer.registrar({ tempoColeta: '2026-01-01T10:00:00.000Z' });
    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });

    await attemptService.handleLiveTelemetry({ status: 'waiting_start', numTentativa: 9001 });

    expect(liveTelemetryBuffer.drenar()).toEqual([]);
    expect(liveTrajetoBuffer.listar()).toEqual([]);

    const afterReset = await attemptService.handleLiveTelemetry({
      ...baseSuccessPayload,
      trajetoAtual: { passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' },
    });

    expect(afterReset.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(2);
  });

  it('persiste tentativa em success de forma idempotente na mesma corrida', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 42 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    liveTelemetryBuffer.registrar({
      tempoColeta: '2026-01-01T10:00:00.000Z',
      posHRecente: 1,
      posVRecente: 0,
    });

    const first = await attemptService.handleLiveTelemetry(baseSuccessPayload);
    const second = await attemptService.handleLiveTelemetry(baseSuccessPayload);

    expect(first.alreadyPersisted).toBe(false);
    expect(second.alreadyPersisted).toBe(true);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(1);
    expect(mockInserirTrajetoEmLote).toHaveBeenCalled();
  });

  it('persiste novamente após running com passo 1 (nova corrida, mesmo espKey)', async () => {
    mockCriarHistorico
      .mockResolvedValueOnce({ numtentativa: 10 })
      .mockResolvedValueOnce({ numtentativa: 11 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    const first = await attemptService.handleLiveTelemetry(baseSuccessPayload);
    expect(first.alreadyPersisted).toBe(false);

    await attemptService.handleLiveTelemetry({
      status: 'running',
      numTentativa: 9001,
      tempoColeta: '2026-01-01T10:01:00.000Z',
      trajetoAtual: { passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' },
    });

    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });
    liveTrajetoBuffer.registrar({ passo: 2, pos_h: 1, pos_v: 0, direcao: 'LESTE' });

    const second = await attemptService.handleLiveTelemetry({
      ...baseSuccessPayload,
      tempoColeta: '2026-01-01T10:02:00.000Z',
    });

    expect(second.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(2);
    expect(mockInserirTrajetoEmLote).toHaveBeenCalledTimes(2);
  });

  it('resetAttemptState permite nova persistência com mesmo espKey', async () => {
    mockCriarHistorico
      .mockResolvedValueOnce({ numtentativa: 20 })
      .mockResolvedValueOnce({ numtentativa: 21 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    await attemptService.handleLiveTelemetry(baseSuccessPayload);
    attemptService.resetAttemptState();

    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });

    const second = await attemptService.handleLiveTelemetry(baseSuccessPayload);

    expect(second.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(2);
  });

  it('persiste mapeamento concluído (waiting_run) com desafioCumprido SIM e trajeto de visitedPath', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 50 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    const payload = {
      status: 'waiting_run',
      numTentativa: 7001,
      mazeSize: 8,
      tipoLabirinto: '8x8',
      batteryPercent: 75,
      speedMps: 0.4,
      tempoColeta: '2026-01-01T10:05:00.000Z',
      visitedPath: [[1, 1], [2, 1], [3, 1]],
      trajetoAtual: { passo: 1, pos_h: 2, pos_v: 0, direcao: 'LESTE' },
    };

    const result = await attemptService.handleLiveTelemetry(payload);

    expect(result.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledWith(
      expect.objectContaining({ desafioCumprido: 'SIM' }),
    );
    expect(mockInserirTrajetoEmLote).toHaveBeenCalledWith(
      expect.arrayContaining([
        expect.objectContaining({ passo: 1, pos_h: 0, pos_v: 0 }),
        expect.objectContaining({ passo: 2, pos_h: 0, pos_v: 1 }),
        expect.objectContaining({ passo: 3, pos_h: 0, pos_v: 2 }),
      ]),
      50,
    );
  });

  it('persiste run travada (stuck) com desafioCumprido NAO', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 51 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    const payload = {
      status: 'stuck',
      numTentativa: 7002,
      mazeSize: 8,
      visitedPath: [[1, 1], [2, 1]],
      tempoColeta: '2026-01-01T10:06:00.000Z',
    };

    const result = await attemptService.handleLiveTelemetry(payload);

    expect(result.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledWith(
      expect.objectContaining({ desafioCumprido: 'NAO' }),
    );
  });

  it('persiste run interrompida (stopped) com desafioCumprido NAO', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 52 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    const payload = {
      status: 'stopped',
      numTentativa: 7003,
      mazeSize: 8,
      visitedPath: [[1, 1]],
      tempoColeta: '2026-01-01T10:07:00.000Z',
    };

    const result = await attemptService.handleLiveTelemetry(payload);

    expect(result.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledWith(
      expect.objectContaining({ desafioCumprido: 'NAO' }),
    );
  });

  it('gera duas entradas para mapeamento e corrida com numeroTentativa diferentes', async () => {
    mockCriarHistorico
      .mockResolvedValueOnce({ numtentativa: 60 })
      .mockResolvedValueOnce({ numtentativa: 61 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    const mapping = await attemptService.handleLiveTelemetry({
      status: 'waiting_run',
      numTentativa: 8001,
      mazeSize: 8,
      visitedPath: [[1, 1], [2, 1]],
      tempoColeta: '2026-01-01T11:00:00.000Z',
    });

    const race = await attemptService.handleLiveTelemetry({
      status: 'success',
      numTentativa: 8002,
      mazeSize: 8,
      visitedPath: [[1, 1], [3, 3]],
      tempoColeta: '2026-01-01T11:10:00.000Z',
    });

    expect(mapping.alreadyPersisted).toBe(false);
    expect(race.alreadyPersisted).toBe(false);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(2);
  });

  it('frames terminais repetidos do mesmo numeroTentativa persistem só uma vez', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 70 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    const payload = {
      status: 'waiting_run',
      numTentativa: 9005,
      mazeSize: 8,
      visitedPath: [[1, 1]],
      tempoColeta: '2026-01-01T12:00:00.000Z',
    };

    const first = await attemptService.handleLiveTelemetry(payload);
    const second = await attemptService.handleLiveTelemetry(payload);

    expect(first.alreadyPersisted).toBe(false);
    expect(second.alreadyPersisted).toBe(true);
    expect(mockCriarHistorico).toHaveBeenCalledTimes(1);
  });

  it('prefere visitedPath completo em vez do buffer esparso de trajetoAtual', async () => {
    mockCriarHistorico.mockResolvedValue({ numtentativa: 80 });
    mockInserirTelemetriaEmLote.mockResolvedValue(undefined);
    mockInserirTrajetoEmLote.mockResolvedValue(undefined);

    liveTrajetoBuffer.registrar({ passo: 5, pos_h: 4, pos_v: 0, direcao: 'LESTE' });

    await attemptService.handleLiveTelemetry({
      ...baseSuccessPayload,
      visitedPath: [[1, 1], [2, 1], [3, 1]],
      trajetoAtual: { passo: 5, pos_h: 4, pos_v: 0, direcao: 'LESTE' },
    });

    expect(mockInserirTrajetoEmLote).toHaveBeenCalledWith(
      expect.arrayContaining([
        expect.objectContaining({ passo: 1, pos_h: 0, pos_v: 0 }),
        expect.objectContaining({ passo: 3, pos_h: 0, pos_v: 2 }),
      ]),
      80,
    );
    expect(mockInserirTrajetoEmLote.mock.calls[0][0]).toHaveLength(3);
  });
});
