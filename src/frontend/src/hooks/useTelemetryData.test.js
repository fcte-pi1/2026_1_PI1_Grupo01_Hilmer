import { describe, it, expect } from 'vitest';
import { renderHook } from '@testing-library/react';
import { useTelemetryData } from './useTelemetryData';

describe('useTelemetryData Hook', () => {
  it('should initialize with default states and without errors', () => {
    const { result } = renderHook(() => useTelemetryData());

    expect(result.current).toBeDefined();
    if (result.current.data?.status) {
      expect(result.current.data.status).toBeTypeOf('string');
    }
  });
});
