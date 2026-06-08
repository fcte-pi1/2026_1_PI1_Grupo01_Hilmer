import { describe, it, expect } from 'vitest';
import { formatTime, formatSpeed, formatBattery, formatMazeDimension, statusLabel } from './helpers';

describe('Helpers Utility Functions', () => {
  it('formatTime should convert seconds into MM:SS format', () => {
    expect(formatTime(65)).toBe('01:05');
    expect(formatTime(9)).toBe('00:09');
    expect(formatTime(3600)).toBe('60:00');
  });

  it('formatSpeed should format number with 2 decimal places and m/s unit', () => {
    expect(formatSpeed(1)).toBe('1.00 m/s');
    expect(formatSpeed(0.555)).toBe('0.56 m/s'); // rounding check
  });

  it('formatBattery should format percent correctly', () => {
    expect(formatBattery(85)).toBe('85%');
  });

  it('formatMazeDimension should format correctly', () => {
    expect(formatMazeDimension(16)).toBe('16x16');
  });
});
