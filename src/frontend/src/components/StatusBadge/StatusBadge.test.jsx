import { render, screen } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { StatusBadge } from './StatusBadge';

vi.mock('../../utils/helpers', () => ({
  statusLabel: vi.fn((status) => `Status: ${status}`)
}));

describe('StatusBadge Component', () => {
  it('should render the badge with the correct class and label', () => {
    render(<StatusBadge status="running" />);

    const badgeElement = screen.getByText('Status: running');
    expect(badgeElement).toBeDefined();
    expect(badgeElement.className).toContain('badge');
  });
});
