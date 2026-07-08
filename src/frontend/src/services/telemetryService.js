// Cliente de telemetria: conecta no backend (broker WebSocket em src/backend)
// e normaliza o payload retransmitido da ESP32 para o formato usado pelos
// componentes do dashboard (MazeView/Sidebar).

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
  };
}

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
    elapsedSeconds: step * 0.8,
    batteryPercent: Math.max(100 - step * 2, 20),
    speedMps: Number((0.45 + (step % 3) * 0.05).toFixed(2)),
    correnteEletrica: 1.2,
    tensaoEletrica: 7.4,
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
  const position =
    raw.position ??
    (Number.isFinite(Number(raw.posVRecente)) && Number.isFinite(Number(raw.posHRecente))
      ? [Number(raw.posVRecente), Number(raw.posHRecente)]
      : null);

  return {
    mazeSize: raw.mazeSize ?? inferMazeSizeFromGrid(grid),
    position,
    visitedPath: raw.visitedPath ?? (position ? [position] : []),
    goal: raw.goal ?? null,
    start: raw.start ?? null,
    grid,
    status: mapRobotStatus(raw.status),
    elapsedSeconds: raw.elapsedSeconds ?? 0,
    batteryPercent: raw.batteryPercent ?? raw.bateriaAtual ?? null,
    speedMps: raw.speedMps ?? raw.velocidadeAtual ?? null,
    correnteEletrica: raw.correnteEletrica ?? raw.correnteRecente ?? null,
    tensaoEletrica: raw.tensaoEletrica ?? raw.tensaoAtual ?? null,
    mode: raw.modoOperacao ?? null,
    attemptNumber: raw.numTentativa ?? null,
    awaitingRun: raw.aguardandoCorrida ?? false,
    stuckFlag: raw.travado ?? false,
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

export function sendStartMappingCommand(socket, mazeSize) {
  return sendCommand(socket, { type: 'START', mazeSize });
}

export function sendStartRaceCommand(socket) {
  return sendCommand(socket, { type: 'START_RUN' });
}

export function sendStopCommand(socket) {
  return sendCommand(socket, { type: 'STOP' });
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
