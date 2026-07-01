// Cliente de telemetria: conecta no backend (broker WebSocket em src/backend)
// e normaliza o payload retransmitido da ESP32 para o formato usado pelos
// componentes do dashboard (MazeView/Sidebar).

const DEFAULT_WS_URL = 'ws://localhost:3001';

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
  };
}

// A ESP32 manda o mapa no campo "mapa" (grid MAZE*2+1 x MAZE*2+1, células
// ímpares = corredores/paredes, célula 0 = visitada, 1 = parede, 2 =
// desconhecido). O tamanho real do labirinto é derivado da dimensão do grid.
export function normalizeTelemetry(raw) {
  const grid = raw.mapa ?? raw.grid ?? null;
  const mazeSize = grid ? (grid.length - 1) / 2 : null;

  return {
    mazeSize,
    position: raw.position ?? null,
    visitedPath: raw.visitedPath ?? [],
    goal: raw.goal ?? null,
    start: raw.start ?? null,
    grid,
    status: raw.status ?? 'running',
    elapsedSeconds: raw.elapsedSeconds ?? 0,
    batteryPercent: raw.batteryPercent ?? null,
    speedMps: raw.speedMps ?? null,
  };
}

// Abre a conexão com o backend e delega os eventos para os callbacks
// informados. Retorna o socket cru, para permitir enviar comandos
// (ex: iniciar corrida) e fechar a conexão no cleanup do hook.
export function connectTelemetrySocket({ onOpen, onClose, onTelemetry }) {
  const socket = new WebSocket(getTelemetryWsUrl());

  socket.onopen = () => onOpen?.();
  socket.onclose = () => onClose?.();
  socket.onerror = () => {}; // erro de conexão é sempre seguido de onclose

  socket.onmessage = (event) => {
    try {
      const raw = JSON.parse(event.data);
      onTelemetry?.(normalizeTelemetry(raw));
    } catch (error) {
      console.error('[telemetria] Payload inválido recebido da ESP32:', error);
    }
  };

  return socket;
}

export function sendStartRaceCommand(socket) {
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send('INICIAR_CORRIDA');
    return true;
  }
  return false;
}
