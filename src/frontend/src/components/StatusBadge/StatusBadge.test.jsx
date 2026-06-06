import { render, screen } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { StatusBadge } from './StatusBadge';

// Criando mock da função auxiliar usada pelo componente
vi.mock('../../utils/helpers', () => ({
  statusLabel: vi.fn((status) => `Status: ${status}`)
}));

describe('StatusBadge Component', () => {
  it('should render the badge with the correct class and label', () => {
    render(<StatusBadge status="running" />);
    
    const badgeElement = screen.getByText('Status: running');
    expect(badgeElement).toBeDefined();
    
    // Verificando a presença de classes. 
    // Em css modules, os nomes não são diretos assim, mas o className conterá informações relevantes.
    expect(badgeElement.className).toContain('badge');
  });
});
