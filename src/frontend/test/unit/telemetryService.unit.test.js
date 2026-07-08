import { describe, it, expect } from 'vitest';
import {
  analysisToMazeViewProps,
  applyStuckFreeze,
  getMockTelemetrySnapshot,
  normalizeTelemetry,
  sanitizeVisitedPath,
} from '../../src/services/telemetryService';

function buildOpenGrid(size = 10) {
  return Array.from({ length: size }, () => Array(size).fill(0));
}

describe('sanitizeVisitedPath', () => {
  it('trunca o path ao encontrar célula de parede', () => {
    const grid = buildOpenGrid(5);
    grid[2][2] = 1;

    const result = sanitizeVisitedPath([[1, 1], [2, 1], [2, 2]], grid);

    expect(result).toEqual([[1, 1], [2, 1]]);
  });

  it('trunca o path em salto não adjacente', () => {
    const grid = buildOpenGrid(5);

    const result = sanitizeVisitedPath([[1, 1], [3, 1]], grid);

    expect(result).toEqual([[1, 1]]);
  });

  it('mantém path válido em células 0 e 2', () => {
    const grid = buildOpenGrid(5);
    grid[2][1] = 2;

    const result = sanitizeVisitedPath([[1, 1], [2, 1], [2, 2]], grid);

    expect(result).toEqual([[1, 1], [2, 1], [2, 2]]);
  });
});

describe('normalizeTelemetry', () => {
  it('prioriza o último ponto do visitedPath quando position conflita', () => {
    const result = normalizeTelemetry({
      visitedPath: [[1, 1], [2, 1], [3, 1]],
      position: [1, 1],
      mapa: buildOpenGrid(10),
    });

    expect(result.visitedPath).toEqual([[1, 1], [2, 1], [3, 1]]);
    expect(result.position).toEqual([3, 1]);
  });

  it('usa posVRecente/posHRecente como fallback quando visitedPath está vazio', () => {
    const result = normalizeTelemetry({
      posVRecente: 2,
      posHRecente: 3,
      mapa: buildOpenGrid(10),
    });

    expect(result.visitedPath).toEqual([[2, 3]]);
    expect(result.position).toEqual([2, 3]);
  });

  it('remove duplicatas consecutivas do visitedPath', () => {
    const result = normalizeTelemetry({
      visitedPath: [[1, 1], [1, 1], [2, 1]],
      mapa: buildOpenGrid(10),
    });

    expect(result.visitedPath).toEqual([[1, 1], [2, 1]]);
    expect(result.position).toEqual([2, 1]);
  });

  it('mapeia travado para status stuck', () => {
    const result = normalizeTelemetry({
      travado: true,
      visitedPath: [[1, 1]],
      mapa: buildOpenGrid(10),
    });

    expect(result.status).toBe('stuck');
    expect(result.stuckFlag).toBe(true);
  });

  it('rejeita avanço fantasma em parede no visitedPath', () => {
    const mapa = buildOpenGrid(5);
    mapa[2][2] = 1;

    const result = normalizeTelemetry({
      visitedPath: [[1, 1], [2, 1], [2, 2]],
      mapa,
    });

    expect(result.visitedPath).toEqual([[1, 1], [2, 1]]);
    expect(result.position).toEqual([2, 1]);
  });
});

describe('applyStuckFreeze', () => {
  it('congela o visitedPath enquanto travado', () => {
    const frozenPathRef = { current: null };
    const stuckTelemetry = {
      status: 'stuck',
      stuckFlag: true,
      start: [1, 1],
      visitedPath: [[1, 1], [2, 1]],
    };

    applyStuckFreeze(stuckTelemetry, frozenPathRef);

    const growingTelemetry = {
      status: 'stuck',
      stuckFlag: true,
      start: [1, 1],
      visitedPath: [[1, 1], [2, 1], [3, 1]],
    };

    const frozen = applyStuckFreeze(growingTelemetry, frozenPathRef);

    expect(frozen.visitedPath).toEqual([[1, 1], [2, 1]]);
    expect(frozen.position).toEqual([2, 1]);
  });

  it('libera o congelamento ao voltar para waiting', () => {
    const frozenPathRef = { current: [[1, 1], [2, 1]] };

    const waitingTelemetry = {
      status: 'waiting',
      stuckFlag: false,
      start: [1, 1],
      visitedPath: [[1, 1]],
    };

    const unfrozen = applyStuckFreeze(waitingTelemetry, frozenPathRef);

    expect(frozenPathRef.current).toBeNull();
    expect(unfrozen.visitedPath).toEqual([[1, 1]]);
  });
});

describe('getMockTelemetrySnapshot', () => {
  it('deriva position do último ponto do visitedPath', () => {
    const snapshot = getMockTelemetrySnapshot(2, 4);

    expect(snapshot.position).toEqual(snapshot.visitedPath.at(-1));
  });
});

describe('analysisToMazeViewProps', () => {
  it('monta props do MazeView a partir da análise', () => {
    const analysis = {
      grid: [
        [1, 1, 1, 1],
        [1, 0, 2, 1],
        [1, 2, 2, 1],
        [1, 1, 1, 1],
      ],
      start: [0, 0],
      goal: [1, 1],
      outboundPath: [
        [0, 0],
        [0, 1],
      ],
      optimalPath: [[0, 0]],
    };

    const props = analysisToMazeViewProps(analysis, 'outboundPath');
    expect(props.visitedPath).toEqual([
      [0, 0],
      [0, 1],
    ]);
    expect(props.position).toEqual([0, 1]);
    expect(props.status).toBe('running');

    const optimalProps = analysisToMazeViewProps(analysis, 'optimalPath');
    expect(optimalProps.status).toBe('success');
  });
});
