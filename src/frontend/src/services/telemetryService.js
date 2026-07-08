// Cliente de telemetria: conecta no backend (broker WebSocket em src/backend)
// e normaliza o payload retransmitido da ESP32 para o formato usado pelos
// componentes do dashboard (MazeView/Sidebar).

import {
  DEFAULT_START_CORNER,
  mirrorGridForCorner,
  mirrorPointForCorner,
  normalizeStartCorner,
} from '../utils/startCorner';

const DEFAULT_WS_URL = 'ws://localhost:3001';

const MOCK_MAZES = {
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
      [1, 1],
      [1, 2],
      [2, 2],
    ],
  },
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
      [1, 1],
      [2, 1],
      [3, 1],
      [4, 1],
      [5, 1],
      [5, 2],
      [5, 3],
      [5, 4],
      [5, 5],
      [5, 6],
      [6, 6],
    ],
  },
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
      [1, 1],
      [2, 1],
      [3, 1],
      [4, 1],
      [5, 1],
      [6, 1],
      [7, 1],
      [7, 2],
      [7, 3],
      [7, 4],
      [7, 5],
      [7, 6],
      [7, 7],
      [8, 7],
      [9, 7],
      [10, 7],
      [11, 7],
      [12, 7],
      [13, 7],
      [14, 7],
      [14, 8],
      [14, 9],
      [14, 10],
      [14, 11],
      [14, 12],
      [14, 13],
      [14, 14],
    ],
  },
};

export function getTelemetryWsUrl() {
  return import.meta.env.VITE_WS_URL || DEFAULT_WS_URL;
}

export function getEmptyTelemetry() {
  return {
    mazeSize: null,
    position: null,
    visitedPath: [],
    goal: null,
    start: null,
    grid: null,
    status: 'idle',
    elapsedSeconds: 0,
    batteryPercent: null,
    speedMps: null,
    correnteEletrica: null,
    tensaoEletrica: null,
    mode: null,
    attemptNumber: null,
    awaitingRun: false,
    stuckFlag: false,
    startCorner: DEFAULT_START_CORNER,
  };
}

export function getMazeMockData(mazeSize, startCorner = DEFAULT_START_CORNER) {
  const maze = MOCK_MAZES[mazeSize] ?? MOCK_MAZES[8];
  const normalizedStartCorner = normalizeStartCorner(startCorner);

  return {
    ...maze,
    startCorner: normalizedStartCorner,
    grid: mirrorGridForCorner(maze.grid, normalizedStartCorner),
    start: mirrorPointForCorner(maze.start, maze.size, normalizedStartCorner),
    goal: mirrorPointForCorner(maze.goal, maze.size, normalizedStartCorner),
    path: maze.path.map((point) => mirrorPointForCorner(point, maze.size, normalizedStartCorner)),
  };
}

function normalizePathPoint(point) {
  if (!Array.isArray(point) || point.length < 2) {
    return null;
  }

  const row = Number(point[0]);
  const col = Number(point[1]);

  if (!Number.isFinite(row) || !Number.isFinite(col)) {
    return null;
  }

  return [row, col];
}

export function resolvePositionFromPath(visitedPath, fallbackStart = null) {
  if (Array.isArray(visitedPath) && visitedPath.length > 0) {
    return visitedPath.at(-1);
  }

  return fallbackStart ?? null;
}

export function resolveVisitedPath(raw, fallbackPosition = null) {
  const rawPath = raw?.visitedPath;

  if (Array.isArray(rawPath) && rawPath.length > 0) {
    const normalized = [];

    for (const point of rawPath) {
      const pathPoint = normalizePathPoint(point);
      if (!pathPoint) {
        continue;
      }

      const last = normalized.at(-1);
      if (last && last[0] === pathPoint[0] && last[1] === pathPoint[1]) {
        continue;
      }

      normalized.push(pathPoint);
    }

    if (normalized.length > 0) {
      return normalized;
    }
  }

  const fallback = normalizePathPoint(fallbackPosition);
  return fallback ? [fallback] : [];
}

function isAdjacentPathPoint(previous, current) {
  return Math.abs(previous[0] - current[0]) + Math.abs(previous[1] - current[1]) === 1;
}

function isPointInsideGrid(row, col, grid) {
  return row >= 0 && row < grid.length && col >= 0 && col < grid[0].length;
}

export function sanitizeVisitedPath(path = [], grid = null) {
  if (!Array.isArray(path) || path.length === 0) {
    return [];
  }

  const sanitized = [];

  for (const point of path) {
    const pathPoint = normalizePathPoint(point);
    if (!pathPoint) {
      break;
    }

    const [row, col] = pathPoint;

    if (grid) {
      if (!isPointInsideGrid(row, col, grid)) {
        break;
      }

      if (grid[row][col] === 1) {
        break;
      }
    }

    const previous = sanitized.at(-1);
    if (previous && !isAdjacentPathPoint(previous, pathPoint)) {
      break;
    }

    sanitized.push(pathPoint);
  }

  return sanitized;
}

export function applyStuckFreeze(telemetry, frozenPathRef) {
  const isStuck = telemetry.status === 'stuck' || telemetry.stuckFlag;

  if (isStuck) {
    if (!frozenPathRef.current) {
      frozenPathRef.current = telemetry.visitedPath;
    }
  } else if (telemetry.status === 'waiting' || telemetry.status === 'idle') {
    frozenPathRef.current = null;
  }

  const visitedPath = frozenPathRef.current ?? telemetry.visitedPath;

  return {
    ...telemetry,
    visitedPath,
    position: resolvePositionFromPath(visitedPath, telemetry.start),
  };
}

function inferFallbackPosition(raw) {
  const fromPosition = normalizePathPoint(raw?.position);
  if (fromPosition) {
    return fromPosition;
  }

  if (
    Number.isFinite(Number(raw?.posVRecente))
    && Number.isFinite(Number(raw?.posHRecente))
  ) {
    return [Number(raw.posVRecente), Number(raw.posHRecente)];
  }

  return null;
}

export function getMockTelemetrySnapshot(stepIndex, mazeSize = 8, startCorner = DEFAULT_START_CORNER) {
  const maze = getMazeMockData(mazeSize, startCorner);
  const step = Math.min(stepIndex, maze.path.length - 1);
  const visitedPath = maze.path.slice(0, step + 1);
  const position = resolvePositionFromPath(visitedPath, maze.start);
  const isFinished = step === maze.path.length - 1;

  return {
    mazeSize: maze.size,
    position,
    visitedPath,
    goal: maze.goal,
    start: maze.start,
    grid: maze.grid,
    status: isFinished ? 'success' : 'running',
    elapsedSeconds: step * 0.8,
    batteryPercent: Math.max(100 - step * 2, 20),
    speedMps: Number((0.45 + (step % 3) * 0.05).toFixed(2)),
    correnteEletrica: 1.2,
    tensaoEletrica: 7.4,
    startCorner: maze.startCorner,
  };
}

// nova alt: a matriz enviada pela ESP32 agora é mazeSize+2 (1 célula real
// = 1 posição na matriz, com 1 camada de parede fixa ao redor) — antes
// era mazeSize*2+1 (resolução dupla, célula e parede em posições
// separadas). Isso só é usado como fallback quando o payload não traz
// `mazeSize` diretamente (o que a ESP32 sempre manda hoje).
function inferMazeSizeFromGrid(grid) {
  if (!grid) {
    return null;
  }

  return grid.length - 2;
}

// A ESP32 manda "waiting_start" (aguardando comando START) e "waiting_run"
// (mapeamento concluído, aguardando START_RUN) — ambos viram 'waiting' pro
// StatusBadge (só tem CSS pra 'waiting' genérico); a distinção entre os dois
// fica no campo awaitingRun, usado pra habilitar a mensagem/botão certos.
function mapRobotStatus(rawStatus) {
  if (rawStatus === 'waiting_start' || rawStatus === 'waiting_run') {
    return 'waiting';
  }

  if (rawStatus === 'stopped') {
    return 'stopped';
  }

  return rawStatus ?? 'running';
}

export function normalizeTelemetry(raw) {
  const grid = raw.mapa ?? raw.grid ?? null;
  const fallbackPosition = inferFallbackPosition(raw);
  const rawPath = resolveVisitedPath(raw, fallbackPosition);
  const visitedPath = sanitizeVisitedPath(rawPath, grid);
  const position = resolvePositionFromPath(visitedPath, raw.start ?? fallbackPosition);
  const mappedStatus = mapRobotStatus(raw.status);

  return {
    mazeSize: raw.mazeSize ?? inferMazeSizeFromGrid(grid),
    position,
    visitedPath,
    goal: raw.goal ?? null,
    start: raw.start ?? null,
    grid,
    status: raw.travado ? 'stuck' : mappedStatus,
    elapsedSeconds: raw.elapsedSeconds ?? 0,
    batteryPercent: raw.batteryPercent ?? raw.bateriaAtual ?? null,
    speedMps: raw.speedMps ?? raw.velocidadeAtual ?? null,
    correnteEletrica: raw.correnteEletrica ?? raw.correnteRecente ?? null,
    tensaoEletrica: raw.tensaoEletrica ?? raw.tensaoAtual ?? null,
    mode: raw.modoOperacao ?? null,
    attemptNumber: raw.numTentativa ?? null,
    awaitingRun: raw.aguardandoCorrida ?? false,
    stuckFlag: raw.travado ?? false,
    startCorner: normalizeStartCorner(raw.startCorner),
  };
}

// Respostas de comando (type: "ACK"/"ERROR") não são telemetria — são só a
// confirmação de que a ESP32 recebeu um START/START_RUN/STOP. Misturá-las
// com normalizeTelemetry() geraria um snapshot falso (sem posição, mapa,
// etc.) e piscaria o dashboard. onCommandAck é opcional; sem ele, só loga.
function isCommandResponse(raw) {
  return raw?.type === 'ACK' || raw?.type === 'ERROR';
}

export function connectTelemetrySocket({ onOpen, onClose, onTelemetry, onCommandAck }) {
  const socket = new WebSocket(getTelemetryWsUrl());

  socket.onopen = () => onOpen?.();
  socket.onclose = () => onClose?.();
  socket.onerror = () => {};

  socket.onmessage = (event) => {
    try {
      const raw = JSON.parse(event.data);

      if (isCommandResponse(raw)) {
        if (raw.type === 'ERROR') {
          console.error('[telemetria] ESP32 rejeitou comando:', raw.message ?? raw);
        }
        onCommandAck?.(raw);
        return;
      }

      onTelemetry?.(normalizeTelemetry(raw));
    } catch (error) {
      console.error('[telemetria] Payload inválido recebido da ESP32:', error);
    }
  };

  return socket;
}

function sendCommand(socket, payload) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify(payload));
    return true;
  }

  return false;
}

export function sendStartMappingCommand(socket, mazeSize, startCorner = DEFAULT_START_CORNER) {
  return sendCommand(socket, {
    type: 'START',
    mazeSize,
    startCorner: normalizeStartCorner(startCorner),
  });
}

export function sendStartRaceCommand(socket) {
  return sendCommand(socket, { type: 'START_RUN' });
}

export function sendStopCommand(socket) {
  return sendCommand(socket, { type: 'STOP' });
}

export function analysisToMazeViewProps(analysis, pathKey = 'outboundPath') {
  const visitedPath = resolveVisitedPath(
    { visitedPath: analysis?.[pathKey] ?? [] },
    analysis?.start ?? null,
  );

  return {
    grid: analysis.grid,
    visitedPath,
    start: analysis.start,
    goal: analysis.goal,
    position: resolvePositionFromPath(visitedPath, analysis.start),
    status: pathKey === 'optimalPath' ? 'success' : 'running',
  };
}
