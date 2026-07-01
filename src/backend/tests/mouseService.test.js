import { describe, it, expect } from 'vitest';
import mouseService from '../src/services/mouseService.js';

describe('mouseService.processFloodFill', () => {
  it('retorna o próximo passo no caminho mais curto', () => {
    const mazeMatrix = [
      [1, 1, 1, 1, 1],
      [1, 0, 0, 0, 1],
      [1, 1, 1, 0, 1],
      [1, 1, 1, 0, 1],
      [1, 1, 1, 1, 1],
    ];

    const result = mouseService.processFloodFill({ row: 1, col: 1 }, mazeMatrix);

    expect(result).toEqual({
      nextPosition: { row: 1, col: 2 },
      action: 'RIGHT',
    });
  });

  it('retorna ALREADY_AT_GOAL quando já inicia no objetivo', () => {
    const mazeMatrix = [
      [1, 1, 1],
      [1, 0, 1],
      [1, 1, 1],
    ];

    const result = mouseService.processFloodFill({ row: 1, col: 1 }, mazeMatrix);

    expect(result).toEqual({
      nextPosition: { row: 1, col: 1 },
      action: 'ALREADY_AT_GOAL',
    });
  });

  it('retorna STUCK quando o objetivo está isolado', () => {
    const mazeMatrix = [
      [1, 1, 1, 1],
      [1, 0, 1, 1],
      [1, 1, 0, 1],
      [1, 1, 1, 1],
    ];

    const result = mouseService.processFloodFill({ row: 1, col: 1 }, mazeMatrix);

    expect(result).toEqual({
      nextPosition: { row: 1, col: 1 },
      action: 'STUCK',
    });
  });
});