import { describe, expect, it } from 'vitest';
import { buildTrajectoryPayload, toCellPath } from './Dashboard.jsx';

describe('toCellPath', () => {
  it('converte visitedPath n+2 para células', () => {
    expect(toCellPath([[1, 1], [2, 1], [3, 2]], 8)).toEqual([
      [0, 0],
      [1, 0],
      [2, 1],
    ]);
  });

  it('converte visitedPath legado 2n+1 para células', () => {
    expect(toCellPath([[1, 1], [3, 1], [5, 3]], 8)).toEqual([
      [0, 0],
      [1, 0],
      [2, 1],
    ]);
  });
});

describe('buildTrajectoryPayload', () => {
  it('grava pos_h/pos_v em coordenadas de célula', () => {
    const payload = buildTrajectoryPayload(10, [[1, 1], [2, 1]], 8);

    expect(payload).toEqual([
      {
        numTentativa: 10,
        passo: 1,
        pos_h: 0,
        pos_v: 0,
        direcao: 'SUL',
      },
      {
        numTentativa: 10,
        passo: 2,
        pos_h: 0,
        pos_v: 1,
        direcao: 'SUL',
      },
    ]);
  });
});
