import { describe, it, expect, vi } from 'vitest';
import { screen, fireEvent, waitFor } from '@testing-library/react';
import { renderApp } from '../../test-utils/renderWithRouter';

const MOCK_HISTORICO = [
  { numtentativa: 1, tipolabirinto: '4x4', velocidademedia: 0.40, percentualbateria: 90, correnteeletrica: 1.1, tensaoeletrica: 7.4, tempoconclusao: '2026-01-01T10:00:00.000Z', desafiocumprido: 'SIM' },
  { numtentativa: 2, tipolabirinto: '4x4', velocidademedia: 0.42, percentualbateria: 88, correnteeletrica: 1.0, tensaoeletrica: 7.3, tempoconclusao: '2026-01-01T10:05:00.000Z', desafiocumprido: 'NAO' },
  { numtentativa: 3, tipolabirinto: '8x8', velocidademedia: 0.50, percentualbateria: 80, correnteeletrica: 1.2, tensaoeletrica: 7.2, tempoconclusao: '2026-01-01T10:10:00.000Z', desafiocumprido: 'SIM' },
  { numtentativa: 4, tipolabirinto: '16x16', velocidademedia: 0.60, percentualbateria: 70, correnteeletrica: 1.3, tensaoeletrica: 7.1, tempoconclusao: '2026-01-01T10:15:00.000Z', desafiocumprido: 'SIM' },
  { numtentativa: 5, tipolabirinto: '8x8', velocidademedia: 0.55, percentualbateria: 60, correnteeletrica: 1.4, tensaoeletrica: 7.0, tempoconclusao: '2026-01-01T10:20:00.000Z', desafiocumprido: 'NAO' },
  { numtentativa: 6, tipolabirinto: '4x4', velocidademedia: 0.45, percentualbateria: 50, correnteeletrica: 1.5, tensaoeletrica: 6.9, tempoconclusao: '2026-01-01T10:25:00.000Z', desafiocumprido: 'SIM' },
];

vi.mock('../../services/apiService', () => ({
  listarHistorico: vi.fn(() => Promise.resolve({ data: MOCK_HISTORICO })),
  analisarTentativa: vi.fn(() => Promise.resolve({ data: null })),
}));

describe('History filter integration', () => {
  it('filters executions by maze dimension', async () => {
    renderApp(['/history']);

    await waitFor(() => {
      expect(screen.getAllByText(/^#\d+$/).length).toBe(6);
    });

    fireEvent.click(screen.getByRole('button', { name: '16×16' }));

    await waitFor(() => {
      expect(screen.getAllByText(/^#\d+$/).length).toBe(1);
    });
    expect(screen.getByText('#4')).toBeInTheDocument();
  });
});
