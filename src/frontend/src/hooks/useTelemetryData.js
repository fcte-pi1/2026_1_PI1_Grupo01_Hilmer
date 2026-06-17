import { useCallback, useEffect, useMemo, useRef, useState } from 'react';
import { getMockTelemetrySnapshot } from '../services/telemetryService';

function getTelemetryWebSocketUrl() {
  if (import.meta.env.VITE_TELEMETRY_WS_URL) {
    return import.meta.env.VITE_TELEMETRY_WS_URL;
  }

  const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  const hostname = window.location.hostname || 'localhost';
  return `${protocol}//${hostname}:3001`;
}

function parseMazeSize(tipoLabirinto, fallback) {
  if (typeof tipoLabirinto !== 'string') return fallback;

  const match = tipoLabirinto.match(/^(\d+)x\1$/);
  return match ? Number(match[1]) : fallback;
}

function toNumber(value, fallback = 0) {
  const numberValue = Number(value);
  return Number.isFinite(numberValue) ? numberValue : fallback;
}

function toCoordinate(value, fallback) {
  if (Array.isArray(value) && value.length >= 2) {
    return [toNumber(value[0], fallback[0]), toNumber(value[1], fallback[1])];
  }

  if (value && typeof value === 'object') {
    const row = value.row ?? value.linha ?? value.x;
    const col = value.col ?? value.coluna ?? value.y;
    return [toNumber(row, fallback[0]), toNumber(col, fallback[1])];
  }

  return fallback;
}

function isFinished(payload) {
  const flag = payload?.desafioCumprido;
  return flag === true || flag === 'S' || flag === 's' || flag === 'true' || flag === 'SIM';
}

function normalizeTelemetry(payload, previousData, mazeSize, run) {
  const grid = Array.isArray(payload.mapa) && Array.isArray(payload.mapa[0])
    ? payload.mapa
    : previousData.grid;
  const rows = grid.length;
  const cols = Array.isArray(grid[0]) ? grid[0].length : rows;
  const start = toCoordinate(payload.inicio ?? payload.start, previousData.start ?? [1, 1]);
  const goal = toCoordinate(
    payload.objetivo ?? payload.goal,
    previousData.goal ?? [Math.floor(rows / 2), Math.floor(cols / 2)],
  );
  const position = toCoordinate(
    payload.posicaoAtual ?? payload.posicao ?? payload.position,
    previousData.position ?? start,
  );
  const previousPath = previousData.visitedPath ?? [];
  const alreadyVisited = previousPath.some(([row, col]) => row === position[0] && col === position[1]);
  const visitedPath = alreadyVisited ? previousPath : [...previousPath, position];

  return {
    mazeSize: parseMazeSize(payload.tipoLabirinto, mazeSize),
    position,
    visitedPath,
    goal,
    start,
    grid,
    status: isFinished(payload) ? 'success' : 'running',
    elapsedSeconds: toNumber(payload.tempoConclusao ?? payload.tempo ?? payload.elapsedSeconds, previousData.elapsedSeconds),
    batteryPercent: toNumber(payload.percentualBateria ?? payload.bateriaConsumo ?? payload.batteryPercent, previousData.batteryPercent),
    speedMps: toNumber(payload.velocidadeMedia ?? payload.speedMps, previousData.speedMps),
    phase: run === 2 ? 'segunda passagem' : 'primeira passagem',
  };
}

export function useTelemetryData(mazeSize = 16, run = 1, initiallyRunning = false) {
  const initialData = useMemo(() => getMockTelemetrySnapshot(0, mazeSize, run), [mazeSize, run]);
  const [data, setData] = useState(initialData);
  const [running, setRunning] = useState(initiallyRunning);
  const [connectionStatus, setConnectionStatus] = useState('disconnected');
  const wsRef = useRef(null);

  useEffect(() => {
    setData(getMockTelemetrySnapshot(0, mazeSize, run));
    setRunning(initiallyRunning);
  }, [mazeSize, run, initiallyRunning]);

  useEffect(() => {
    const ws = new WebSocket(getTelemetryWebSocketUrl());
    wsRef.current = ws;

    ws.addEventListener('open', () => {
      setConnectionStatus('connected');
    });

    ws.addEventListener('message', (event) => {
      try {
        const payload = JSON.parse(event.data);
        setRunning(true);
        setData((currentData) => normalizeTelemetry(payload, currentData, mazeSize, run));
      } catch {
        setConnectionStatus('invalid-message');
      }
    });

    ws.addEventListener('close', () => {
      setConnectionStatus('disconnected');
      setRunning(false);
    });

    ws.addEventListener('error', () => {
      setConnectionStatus('error');
    });

    return () => {
      ws.close();
      wsRef.current = null;
    };
  }, [mazeSize, run]);

  const start = useCallback(() => {
    setRunning(true);
    setData((currentData) => ({
      ...currentData,
      status: currentData.status === 'success' ? 'running' : currentData.status,
    }));
  }, []);

  const reset = useCallback(() => {
    setRunning(false);
    setData(getMockTelemetrySnapshot(0, mazeSize, run));
  }, [mazeSize, run]);

  return { data, running, connectionStatus, start, reset };
}
