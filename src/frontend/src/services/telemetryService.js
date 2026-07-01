// MOCK DATA — dados temporários para prototipação visual.
// Este arquivo será substituído pela integração com o backend via WebSocket/HTTP.

const MOCK_MAZES = {
  // 4×4 real — grid simples, caminho direto
  4: {
    size: 4,
    start: [1, 1],
    goal: [2, 2],
    grid: [
      [1, 1, 1, 1],
      [1, 0, 0, 1],
      [1, 1, 0, 1],
      [1, 1, 1, 1],
    ],
    path: [
      [1, 1], [1, 2], [2, 2],
    ],
  },

  // 8×8 real — reaproveitando o grid 10×10 antigo não funciona (tamanho errado),
  // então usamos um grid 8×8 próprio
  8: {
    size: 8,
    start: [1, 1],
    goal: [6, 6],
    grid: [
      [1, 1, 1, 1, 1, 1, 1, 1],
      [1, 0, 0, 0, 1, 0, 0, 1],
      [1, 0, 1, 0, 1, 0, 1, 1],
      [1, 0, 1, 0, 0, 0, 0, 1],
      [1, 0, 1, 1, 1, 1, 0, 1],
      [1, 0, 0, 0, 0, 0, 0, 1],
      [1, 1, 1, 0, 1, 1, 0, 1],
      [1, 1, 1, 1, 1, 1, 1, 1],
    ],
    path: [
      [1, 1], [2, 1], [3, 1], [4, 1], [5, 1],
      [5, 2], [5, 3], [5, 4], [5, 5], [5, 6],
      [6, 6],
    ],
  },

  // 16×16 real — reaproveitando o grid 16×16 que já existia
  16: {
    size: 16,
    start: [1, 1],
    goal: [14, 14],
    grid: [
      [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
      [1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1],
      [1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1],
      [1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1],
      [1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 0, 1, 1],
      [1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1],
      [1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1],
      [1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1],
      [1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1],
      [1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1],
      [1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1],
      [1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 1],
      [1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1],
      [1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1],
      [1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1],
      [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1],
    ],
    path: [
      [1, 1], [2, 1], [3, 1], [4, 1], [5, 1], [6, 1], [7, 1],
      [7, 2], [7, 3], [7, 4], [7, 5], [7, 6], [7, 7],
      [8, 7], [9, 7], [10, 7], [11, 7], [12, 7], [13, 7], [14, 7],
      [14, 8], [14, 9], [14, 10], [14, 11], [14, 12], [14, 13], [14, 14],
    ],
  },
};

export function getMazeMockData(mazeSize) {
  return MOCK_MAZES[mazeSize] ?? MOCK_MAZES[8];
}

export function getMockTelemetrySnapshot(stepIndex, mazeSize = 8) {
  const maze = getMazeMockData(mazeSize);
  const step = Math.min(stepIndex, maze.path.length - 1);
  const position = maze.path[step];
  const visitedPath = maze.path.slice(0, step + 1);
  const isFinished = step === maze.path.length - 1;

  return {
    mazeSize: maze.size,
    position,
    visitedPath,
    goal: maze.goal,
    start: maze.start,
    grid: maze.grid,
    status: isFinished ? 'success' : 'running',
    elapsedSeconds: step,
    batteryPercent: Math.max(85 - step * 2, 40),
    speedMps: 0.45 + Math.random() * 0.1,
  };
}

export function analysisToMazeViewProps(analysis, pathKey = 'outboundPath') {
  const visitedPath = analysis?.[pathKey] ?? [];

  return {
    grid: analysis.grid,
    visitedPath,
    start: analysis.start,
    goal: analysis.goal,
    position: visitedPath[visitedPath.length - 1] ?? analysis.start,
    status: pathKey === 'optimalPath' ? 'success' : 'running',
  };
}