import { describe, it, expect } from 'vitest';
import mouseService from '../../src/services/mouseService.js';
import { telemetria4x4, trajeto4x4 } from '../fixtures/analysisAttempt.js';

describe('mouseService.analyzeAttempt', () => {
  it('separa ida/volta e escolhe caminho ótimo', () => {
    const analysis = mouseService.analyzeAttempt(trajeto4x4, telemetria4x4, '4x4');

    expect(analysis.outboundPath.length).toBeGreaterThan(0);
    expect(analysis.optimalPath.length).toBeGreaterThan(0);
    expect(analysis.optimalPath.length).toBeLessThanOrEqual(
      Math.max(analysis.outboundSteps, analysis.returnSteps),
    );
  });

  it('retorna grid com células 0, 1 e 2', () => {
    const analysis = mouseService.analyzeAttempt(trajeto4x4, telemetria4x4, '8x8');
    const flat = analysis.grid.flat();

    expect(flat).toContain(0);
    expect(flat).toContain(1);
    expect(flat).toContain(2);
  });

  it('buildCellGrid marca células visitadas e desconhecidas', () => {
    const steps = [
      {
        passo: 1,
        pos_h: 1,
        pos_v: 1,
        direcao: 'NORTE',
        sensores: { frontal: 300, esquerda: 300, direita: 300 },
      },
    ];

    const grid = mouseService.buildCellGrid(steps, 8);
    expect(grid[1][1]).toBe(0);
    expect(grid[0][0]).toBe(1);
    expect(grid[2][2]).toBe(2);
  });

  it('buildCellGrid marca parede quando sensor direita está abaixo do limiar', () => {
    const steps = [
      {
        passo: 1,
        pos_h: 0,
        pos_v: 0,
        direcao: 'LESTE',
        sensores: { frontal: 300, esquerda: 300, direita: 50 },
      },
    ];

    const grid = mouseService.buildCellGrid(steps, 4);
    expect(grid[0][0]).toBe(0);
    expect(grid[1][0]).toBe(1);
  });

  it('selectShorterPath retorna o menor caminho', () => {
    expect(mouseService.selectShorterPath([[0, 0], [0, 1], [0, 2]], [[0, 0]])).toEqual([[0, 0]]);
  });
});
