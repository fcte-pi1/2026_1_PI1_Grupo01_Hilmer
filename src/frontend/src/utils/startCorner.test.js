import { describe, expect, it } from 'vitest';
import {
  formatStartCorner,
  mirrorGridForCorner,
  mirrorPointForCorner,
  normalizeStartCorner,
} from './startCorner';

describe('startCorner utils', () => {
  it('normaliza canto inválido para o padrão', () => {
    expect(normalizeStartCorner('lado-esquerdo')).toBe('top-left');
  });

  it('espelha pontos para o canto inferior direito', () => {
    expect(mirrorPointForCorner([1, 2], 8, 'bottom-right')).toEqual([6, 5]);
  });

  it('espelha grid horizontal e verticalmente', () => {
    const grid = [
      [1, 2],
      [3, 4],
    ];

    expect(mirrorGridForCorner(grid, 'bottom-right')).toEqual([
      [4, 3],
      [2, 1],
    ]);
  });

  it('formata o rótulo do canto para a UI', () => {
    expect(formatStartCorner('top-right')).toBe('Superior direito');
  });
});
