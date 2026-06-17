// MOCK DATA — dados temporários para prototipação visual.
// Este arquivo será substituído por chamadas reais à API REST do backend.

export const MOCK_EXECUTION_HISTORY = [
  {
    id: 1,
    attempt: 1,
    mazeSize: 4,
    totalTimeSeconds: 42,
    avgSpeedMps: 0.48,
    totalBatteryUsed: 18,
    status: 'success',
  },
  {
    id: 2,
    attempt: 2,
    mazeSize: 8,
    totalTimeSeconds: 31,
    avgSpeedMps: 0.55,
    totalBatteryUsed: 14,
    status: 'success',
  },
  {
    id: 3,
    attempt: 3,
    mazeSize: 16,
    totalTimeSeconds: 0,
    avgSpeedMps: 0,
    totalBatteryUsed: 5,
    status: 'failure',
  },
  {
    id: 4,
    attempt: 4,
    mazeSize: 16,
    totalTimeSeconds: 87,
    avgSpeedMps: 0.41,
    totalBatteryUsed: 32,
    status: 'success',
  },
  {
    id: 5,
    attempt: 5,
    mazeSize: 4,
    totalTimeSeconds: 0,
    avgSpeedMps: 0,
    totalBatteryUsed: 8,
    status: 'failure',
  },
  {
    id: 6,
    attempt: 6,
    mazeSize: 8,
    totalTimeSeconds: 134,
    avgSpeedMps: 0.38,
    totalBatteryUsed: 47,
    status: 'success',
  },
];

export function getExecutionHistory() {
  return Promise.resolve(MOCK_EXECUTION_HISTORY);
}

export function getExecutionById(id) {
  const execution = MOCK_EXECUTION_HISTORY.find((e) => e.id === id);
  return Promise.resolve(execution ?? null);
}
