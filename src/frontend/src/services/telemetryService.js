// MOCK DATA — dados temporários para prototipação visual.
// Este arquivo será substituído pela integração com o backend via WebSocket/HTTP.

export const MOCK_MAZE_SIZE = 10;

// 1 = parede, 0 = caminho aberto
export const MOCK_MAZE_GRID = [
  [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
  [1, 0, 0, 0, 1, 0, 0, 0, 0, 1],
  [1, 0, 1, 0, 1, 0, 1, 1, 0, 1],
  [1, 0, 1, 0, 0, 0, 0, 1, 0, 1],
  [1, 0, 1, 1, 1, 1, 0, 1, 0, 1],
  [1, 0, 0, 0, 0, 1, 0, 0, 0, 1],
  [1, 1, 1, 0, 1, 1, 1, 1, 0, 1],
  [1, 0, 0, 0, 0, 0, 0, 0, 0, 1],
  [1, 0, 1, 1, 1, 0, 1, 1, 0, 1],
  [1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
];

// Caminho percorrido pelo micromouse (coordenadas [linha, coluna])
export const MOCK_MOUSE_PATH = [
  [1, 1], [1, 2], [1, 3],
  [2, 3], [3, 3], [3, 4], [3, 5], [3, 6],
  [4, 6], [5, 6], [5, 7], [5, 8],
  [6, 8], [7, 8], [8, 8],
];

export const MOCK_START = [1, 1];
export const MOCK_GOAL  = [8, 8];

export function getMockTelemetrySnapshot(stepIndex) {
  const step = Math.min(stepIndex, MOCK_MOUSE_PATH.length - 1);
  const position = MOCK_MOUSE_PATH[step];
  const visitedPath = MOCK_MOUSE_PATH.slice(0, step + 1);
  const isFinished = step === MOCK_MOUSE_PATH.length - 1;

  return {
    mazeSize: MOCK_MAZE_SIZE,
    position,
    visitedPath,
    goal: MOCK_GOAL,
    start: MOCK_START,
    grid: MOCK_MAZE_GRID,
    status: isFinished ? 'success' : 'running',
    elapsedSeconds: step,
    batteryPercent: Math.max(85 - step * 2, 40),
    speedMps: 0.45 + Math.random() * 0.1,
  };
}
