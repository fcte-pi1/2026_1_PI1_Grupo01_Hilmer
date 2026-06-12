import { afterEach, describe, expect, it, vi } from 'vitest';

const queryMock = vi.fn();

vi.mock('../src/database/connection.js', () => ({
  default: {
    query: queryMock,
  },
}));

const simulationService = await import('../src/services/simulationService.js');

afterEach(() => {
  queryMock.mockReset();
});

describe('simulationService', () => {
  it('criarHistorico monta o INSERT sem numTentativa quando o banco deve gerar o id', async () => {
    queryMock.mockResolvedValueOnce({ rows: [{ numtentativa: 12 }] });

    const payload = {
      percentualBateria: 90,
      velocidadeMedia: 0.45,
      tempoConclusao: '2026-06-08T12:00:00.000Z',
      desafioCumprido: 'SIM',
      correnteEletrica: 1.2,
      tensaoEletrica: 7.4,
      tipoLabirinto: '16x16',
    };

    const result = await simulationService.default.criarHistorico(payload);

    expect(queryMock).toHaveBeenCalledWith(
      expect.stringContaining('INSERT INTO HISTORICO'),
      [
        90,
        0.45,
        '2026-06-08T12:00:00.000Z',
        'SIM',
        1.2,
        7.4,
        '16x16',
      ]
    );
    expect(result).toEqual({ numtentativa: 12 });
  });

  it('inserirTelemetria preenche campos opcionais com null', async () => {
    queryMock.mockResolvedValueOnce({ rows: [{ numtentativa: 12 }] });

    await simulationService.default.inserirTelemetria({
      numTentativa: 12,
      tempoColeta: '2026-06-08T12:00:00.000Z',
      tensaoRecente: 7.3,
      correnteRecente: 1.1,
      posHRecente: 0,
      posVRecente: 0,
      velocidadeAtual: 0.5,
      bateriaAtual: 88,
      tensaoAtual: 7.4,
    });

    expect(queryMock).toHaveBeenCalledWith(
      expect.stringContaining('INSERT INTO TELEMETRIA'),
      [12, '2026-06-08T12:00:00.000Z', 7.3, 1.1, 0, 0, 0.5, 88, 7.4, null, null, null, null]
    );
  });

  it('listarTrajetoPorTentativa consulta ordenando por passo', async () => {
    queryMock.mockResolvedValueOnce({ rows: [{ passo: 1 }, { passo: 2 }] });

    const result = await simulationService.default.listarTrajetoPorTentativa(7);

    expect(queryMock).toHaveBeenCalledWith(
      expect.stringContaining('ORDER BY passo ASC'),
      [7]
    );
    expect(result).toEqual([{ passo: 1 }, { passo: 2 }]);
  });
});
