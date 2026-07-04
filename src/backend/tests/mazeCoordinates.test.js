import { describe, expect, it } from 'vitest';
import {
  MAP_FORMAT,
  detectVisitedPathFormat,
  visitedPathToCellSteps,
} from '../src/utils/mazeCoordinates.js';

describe('detectVisitedPathFormat', () => {
  it('identifica mapa bordered (n+2) com coordenadas pares', () => {
    expect(detectVisitedPathFormat([[1, 1], [2, 1], [3, 2]], 8)).toBe(MAP_FORMAT.BORDERED);
  });

  it('identifica mapa corridor com passos de 2', () => {
    expect(detectVisitedPathFormat([[1, 1], [3, 1], [5, 3]], 8)).toBe(MAP_FORMAT.CORRIDOR);
  });
});

describe('visitedPathToCellSteps', () => {
  it('converte mapa bordered (n+2) para célula', () => {
    const steps = visitedPathToCellSteps([[1, 1], [2, 1], [3, 2]], 8);

    expect(steps).toEqual([
      { passo: 1, pos_h: 0, pos_v: 0, direcao: 'SUL' },
      { passo: 2, pos_h: 0, pos_v: 1, direcao: 'SUL' },
      { passo: 3, pos_h: 1, pos_v: 2, direcao: 'SUL' },
    ]);
  });

  it('converte mapa corridor para célula', () => {
    const steps = visitedPathToCellSteps([[1, 1], [3, 1], [5, 3]], 8);

    expect(steps[0]).toMatchObject({ pos_h: 0, pos_v: 0 });
    expect(steps[1]).toMatchObject({ pos_h: 0, pos_v: 1 });
    expect(steps[2]).toMatchObject({ pos_h: 1, pos_v: 2 });
  });
});
