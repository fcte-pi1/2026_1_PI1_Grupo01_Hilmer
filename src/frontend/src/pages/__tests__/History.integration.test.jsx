import { describe, it, expect } from 'vitest';
import { screen, fireEvent, waitFor } from '@testing-library/react';
import { renderApp } from '../../test-utils/renderWithRouter';

describe('History filter integration', () => {
  it('filters executions by maze dimension', async () => {
    renderApp(['/history']);

    await waitFor(() => {
      expect(screen.getAllByText(/Tentativa/).length).toBe(6);
    });

    fireEvent.click(screen.getByRole('button', { name: 'Filtros' }));
    fireEvent.click(screen.getByLabelText('16x16'));

    await waitFor(() => {
      expect(screen.getAllByText(/Tentativa/).length).toBe(1);
    });
    expect(screen.getByText('#4')).toBeInTheDocument();
  });
});
