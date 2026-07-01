import { describe, it, expect } from 'vitest';
import { formatTime, formatSpeed, formatBattery, formatMazeDimension, mazeSizeToTipoLabirinto, statusLabel} from './helpers';
import {
  formatTime,
  formatSpeed,
  formatBattery,
  formatMazeDimension,
  filterExecutions,
} from './helpers';
import { MOCK_EXECUTION_HISTORY } from '../services/dataService';

const emptyFilters = {
  mazeSize: [],
  totalTime: [],
  avgSpeed: [],
  totalBatteryUsed: [],
  status: [],
};

describe('Helpers Utility Functions', () => {
  it('formatTime should convert seconds into MM:SS format', () => {
    expect(formatTime(65)).toBe('01:05');
    expect(formatTime(9)).toBe('00:09');
    expect(formatTime(3600)).toBe('60:00');
  });

  it('formatSpeed should format number with 2 decimal places and m/s unit', () => {
    expect(formatSpeed(1)).toBe('1.00 m/s');
    expect(formatSpeed(0.555)).toBe('0.56 m/s');
  });

  it('formatBattery should format percent correctly', () => {
    expect(formatBattery(85)).toBe('85%');
  });

  it('formatMazeDimension should format correctly', () => {
    expect(formatMazeDimension(16)).toBe('16x16');
  });

  it('mazeSizeToTipoLabirinto should map sizes to the expected schema values', () => {
    expect(mazeSizeToTipoLabirinto(4)).toBe('4x4');
    expect(mazeSizeToTipoLabirinto(8)).toBe('8x8');
    expect(mazeSizeToTipoLabirinto(16)).toBe('16x16');
  });
});

describe('filterExecutions', () => {
  it('returns all executions when filters are empty', () => {
    expect(filterExecutions(MOCK_EXECUTION_HISTORY, emptyFilters)).toHaveLength(6);
  });

  it('filters by maze dimension', () => {
    const result = filterExecutions(MOCK_EXECUTION_HISTORY, {
      ...emptyFilters,
      mazeSize: ['16x16'],
    });
    expect(result).toHaveLength(1);
    expect(result[0].mazeSize).toBe(16);
  });

  it('filters by status Sucesso', () => {
    const result = filterExecutions(MOCK_EXECUTION_HISTORY, {
      ...emptyFilters,
      status: ['Sucesso'],
    });
    expect(result.every((e) => e.status === 'success')).toBe(true);
    expect(result).toHaveLength(4);
  });

  it('filters by total time range', () => {
    const result = filterExecutions(MOCK_EXECUTION_HISTORY, {
      ...emptyFilters,
      totalTime: ['[1s - 59 s]'],
    });
    expect(result.every((e) => e.totalTimeSeconds >= 1 && e.totalTimeSeconds <= 59)).toBe(true);
  });

  it('filters by average speed range', () => {
    const result = filterExecutions(MOCK_EXECUTION_HISTORY, {
      ...emptyFilters,
      avgSpeed: ['[0.01 m/s - 0.59 m/s]'],
    });
    expect(result.every((e) => e.avgSpeedMps >= 0.01 && e.avgSpeedMps <= 0.59)).toBe(true);
  });

  it('filters by battery consumption', () => {
    const result = filterExecutions(MOCK_EXECUTION_HISTORY, {
      ...emptyFilters,
      totalBatteryUsed: ['menor do que 50%'],
    });
    expect(result.every((e) => e.totalBatteryUsed < 50)).toBe(true);
  });

  it('applies multiple filters together', () => {
    const result = filterExecutions(MOCK_EXECUTION_HISTORY, {
      ...emptyFilters,
      mazeSize: ['10x10', '12x12'],
      status: ['Sucesso'],
    });
    expect(result).toHaveLength(2);
    expect(result.map((e) => e.mazeSize)).toEqual([10, 12]);
  });
});
