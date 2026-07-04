import { afterEach, describe, expect, it } from 'vitest';
import liveTrajetoBuffer from '../src/services/liveTrajetoBuffer.js';

afterEach(() => {
  liveTrajetoBuffer.limpar();
});

describe('liveTrajetoBuffer', () => {
  it('inicia vazio', () => {
    expect(liveTrajetoBuffer.listar()).toEqual([]);
  });

  it('registra e lista passos ordenados', () => {
    liveTrajetoBuffer.registrar({ passo: 2, pos_h: 1, pos_v: 0, direcao: 'LESTE' });
    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });

    expect(liveTrajetoBuffer.listar()).toEqual([
      { passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' },
      { passo: 2, pos_h: 1, pos_v: 0, direcao: 'LESTE' },
    ]);
  });

  it('deduplica passo mantendo o mais recente', () => {
    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });
    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 1, pos_v: 0, direcao: 'LESTE' });

    expect(liveTrajetoBuffer.listar()).toEqual([
      { passo: 1, pos_h: 1, pos_v: 0, direcao: 'LESTE' },
    ]);
  });

  it('ignora registro sem passo válido', () => {
    liveTrajetoBuffer.registrar({ pos_h: 0, pos_v: 0 });
    liveTrajetoBuffer.registrar({ passo: 0, pos_h: 0, pos_v: 0 });

    expect(liveTrajetoBuffer.listar()).toEqual([]);
  });

  it('limpa buffer', () => {
    liveTrajetoBuffer.registrar({ passo: 1, pos_h: 0, pos_v: 0, direcao: 'NORTE' });
    liveTrajetoBuffer.limpar();
    expect(liveTrajetoBuffer.listar()).toEqual([]);
  });
});
