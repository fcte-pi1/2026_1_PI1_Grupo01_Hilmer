import { describe, it, expect } from 'vitest';
import { getExecutionHistory, getExecutionById, MOCK_EXECUTION_HISTORY } from './dataService';

describe('dataService', () => {
  it('should return all execution history', async () => {
    const history = await getExecutionHistory();
    expect(history).toBeDefined();
    expect(history.length).toBe(MOCK_EXECUTION_HISTORY.length);
    expect(history[0]).toHaveProperty('id');
    expect(history[0]).toHaveProperty('status');
  });

  it('should return a specific execution by ID', async () => {
    const execution = await getExecutionById(1);
    expect(execution).toBeDefined();
    expect(execution.id).toBe(1);
    expect(execution.attempt).toBe(1);
  });

  it('should return null when execution ID does not exist', async () => {
    const execution = await getExecutionById(999);
    expect(execution).toBeNull();
  });
});
