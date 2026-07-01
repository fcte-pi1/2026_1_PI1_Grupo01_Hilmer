import { render, screen, fireEvent } from '@testing-library/react';
import { describe, it, expect, vi } from 'vitest';
import { Filter } from './Filter';

const emptyFilters = {
  mazeSize: [],
  totalTime: [],
  avgSpeed: [],
  totalBatteryUsed: [],
  status: [],
};

describe('Filter', () => {
  it('does not show backdrop when closed', () => {
    const { container } = render(
      <Filter
        isOpen={false}
        onClose={() => {}}
        filters={emptyFilters}
        setFilters={() => {}}
      />
    );

    expect(container.querySelector('[class*="backdrop"]')).toBeNull();
  });

  it('shows filter options when open', () => {
    render(
      <Filter
        isOpen={true}
        onClose={() => {}}
        filters={emptyFilters}
        setFilters={() => {}}
      />
    );

    expect(screen.getByText('Filtros')).toBeInTheDocument();
    expect(screen.getByText('DIMENSÃO')).toBeInTheDocument();
    expect(screen.getByLabelText('16x16')).toBeInTheDocument();
  });

  it('calls setFilters when checkbox is toggled', () => {
    const setFilters = vi.fn();
    render(
      <Filter
        isOpen={true}
        onClose={() => {}}
        filters={emptyFilters}
        setFilters={setFilters}
      />
    );

    fireEvent.click(screen.getByLabelText('16x16'));
    expect(setFilters).toHaveBeenCalled();
  });

  it('calls onClose when close button is clicked', () => {
    const onClose = vi.fn();
    render(
      <Filter
        isOpen={true}
        onClose={onClose}
        filters={emptyFilters}
        setFilters={() => {}}
      />
    );

    fireEvent.click(screen.getByRole('button', { name: '✕' }));
    expect(onClose).toHaveBeenCalledOnce();
  });

  it('calls onClose when backdrop is clicked', () => {
    const onClose = vi.fn();
    const { container } = render(
      <Filter
        isOpen={true}
        onClose={onClose}
        filters={emptyFilters}
        setFilters={() => {}}
      />
    );

    const backdrop = container.querySelector('[class*="backdrop"]');
    fireEvent.click(backdrop);
    expect(onClose).toHaveBeenCalledOnce();
  });
});
