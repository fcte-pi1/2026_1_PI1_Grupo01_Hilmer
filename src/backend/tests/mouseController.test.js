import { beforeEach, describe, expect, it, vi } from 'vitest';

const processFloodFillMock = vi.fn();

vi.mock('../src/services/mouseService.js', () => ({
  default: {
    processFloodFill: processFloodFillMock,
  },
}));

const { default: mouseController } = await import('../src/controllers/mouseController.js');

describe('mouseController.calculateNextMove', () => {
  beforeEach(() => {
    processFloodFillMock.mockReset();
  });

  it('responde 400 quando currentPosition e mazeMatrix faltam', () => {
    const req = { body: {} };
    const res = createResponseMock();

    mouseController.calculateNextMove(req, res, vi.fn());

    expect(res.status).toHaveBeenCalledWith(400);
    expect(res.json).toHaveBeenCalledWith({
      error: 'currentPosition e mazeMatrix são obrigatórios.',
    });
  });

  it('responde 400 quando currentPosition é inválido', () => {
    const req = { body: { currentPosition: { row: 'a', col: 1 }, mazeMatrix: [[0]] } };
    const res = createResponseMock();

    mouseController.calculateNextMove(req, res, vi.fn());

    expect(res.status).toHaveBeenCalledWith(400);
    expect(res.json).toHaveBeenCalledWith({
      error: 'currentPosition deve ter campos { row: number, col: number }.',
    });
  });

  it('responde 200 com o próximo movimento calculado', () => {
    processFloodFillMock.mockReturnValue({
      nextPosition: { row: 1, col: 2 },
      action: 'RIGHT',
    });

    const req = {
      body: {
        currentPosition: { row: 1, col: 1 },
        mazeMatrix: [[1, 1], [1, 0]],
      },
    };
    const res = createResponseMock();

    mouseController.calculateNextMove(req, res, vi.fn());

    expect(processFloodFillMock).toHaveBeenCalledWith(
      { row: 1, col: 1 },
      [[1, 1], [1, 0]]
    );
    expect(res.status).toHaveBeenCalledWith(200);
    expect(res.json).toHaveBeenCalledWith({
      success: true,
      nextPosition: { row: 1, col: 2 },
      action: 'RIGHT',
    });
  });
});

function createResponseMock() {
  const res = {};
  res.status = vi.fn().mockReturnValue(res);
  res.json = vi.fn().mockReturnValue(res);
  return res;
}