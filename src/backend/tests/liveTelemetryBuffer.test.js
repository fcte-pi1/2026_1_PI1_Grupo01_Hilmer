import { afterEach, describe, expect, it } from 'vitest';
import liveTelemetryBuffer from '../src/services/liveTelemetryBuffer.js';

afterEach(() => {
  liveTelemetryBuffer.limpar();
});

describe('liveTelemetryBuffer', () => {
  it('drenar retorna vazio quando nada foi registrado', () => {
    expect(liveTelemetryBuffer.drenar()).toEqual([]);
  });

  it('registrar acumula snapshots e drenar esvazia o buffer', () => {
    liveTelemetryBuffer.registrar({
      tempoColeta: '2026-01-01T10:00:00.000Z',
      tensaoRecente: 7.2,
      correnteRecente: 1.1,
      posHRecente: 1,
      posVRecente: 1,
      velocidadeAtual: 0.5,
      bateriaAtual: 90,
      tensaoAtual: 7.4,
    });
    liveTelemetryBuffer.registrar({
      tempoColeta: '2026-01-01T10:00:01.000Z',
      tensaoRecente: 7.1,
      correnteRecente: 1.0,
      posHRecente: 1,
      posVRecente: 2,
      velocidadeAtual: 0.55,
      bateriaAtual: 89,
      tensaoAtual: 7.3,
    });

    const drenado = liveTelemetryBuffer.drenar();
    expect(drenado).toHaveLength(2);
    expect(drenado.map((s) => s.tempoColeta)).toEqual([
      '2026-01-01T10:00:00.000Z',
      '2026-01-01T10:00:01.000Z',
    ]);
    expect(liveTelemetryBuffer.drenar()).toEqual([]);
  });

  it('registrar com o mesmo tempoColeta substitui a leitura anterior (dedup por segundo)', () => {
    liveTelemetryBuffer.registrar({ tempoColeta: '2026-01-01T10:00:00.000Z', posHRecente: 1, posVRecente: 1 });
    liveTelemetryBuffer.registrar({ tempoColeta: '2026-01-01T10:00:00.000Z', posHRecente: 2, posVRecente: 2 });

    const drenado = liveTelemetryBuffer.drenar();
    expect(drenado).toHaveLength(1);
    expect(drenado[0]).toMatchObject({ posHRecente: 2, posVRecente: 2 });
  });

  it('registrar ignora payloads sem tempoColeta', () => {
    liveTelemetryBuffer.registrar({ posHRecente: 1, posVRecente: 1 });
    expect(liveTelemetryBuffer.drenar()).toEqual([]);
  });

  it('limpar descarta tudo o que estava no buffer', () => {
    liveTelemetryBuffer.registrar({ tempoColeta: '2026-01-01T10:00:00.000Z' });
    liveTelemetryBuffer.limpar();
    expect(liveTelemetryBuffer.drenar()).toEqual([]);
  });
});
