import { render, screen, waitFor } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { MemoryRouter } from 'react-router-dom';
import { describe, it, expect, vi, beforeEach } from 'vitest';
import { History } from '../../src/pages/History';

vi.mock('../../src/components/MazeView/MazeView', () => ({
  MazeView: () => <div data-testid="maze-view" />,
}));

vi.mock('../../src/services/apiService', () => ({
  listarHistorico: vi.fn(),
  analisarTentativa: vi.fn(),
}));

import { analisarTentativa, listarHistorico } from '../../src/services/apiService';

const fakeAttempt = {
  numtentativa: 42,
  tipolabirinto: '4x4',
  velocidademedia: 0.5,
  percentualbateria: 90,
  correnteeletrica: 1.1,
  tensaoeletrica: 7.4,
  tempoconclusao: '2026-06-08T12:00:00.000Z',
  desafiocumprido: 'SIM',
};

const fakeAnalysis = {
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

function renderHistory() {
  return render(
    <MemoryRouter initialEntries={['/history']}>
      <History />
    </MemoryRouter>,
  );
}

describe('History', () => {
  beforeEach(() => {
    listarHistorico.mockResolvedValue({ data: [fakeAttempt] });
    analisarTentativa.mockResolvedValue({ data: fakeAnalysis });
  });

  it('exibe Primeiro caminho e Caminho ótimo ao selecionar tentativa', async () => {
    const user = userEvent.setup();
    renderHistory();

    await waitFor(() => {
      expect(screen.getByText('#42')).toBeInTheDocument();
    });

    await user.click(screen.getByTestId('history-row-42'));

    await waitFor(() => {
      expect(screen.getByRole('heading', { name: 'Primeiro caminho' })).toBeInTheDocument();
      expect(screen.getByRole('heading', { name: 'Caminho ótimo' })).toBeInTheDocument();
    });

    expect(screen.getAllByTestId('maze-view')).toHaveLength(2);
    expect(analisarTentativa).toHaveBeenCalledWith(42);
  });
});
