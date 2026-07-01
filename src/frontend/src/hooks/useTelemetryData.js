/**
 * Hook de telemetria. O modo é controlado por VITE_TELEMETRY_MODE:
 * mock para desenvolvimento sem hardware, live para ESP32 via backend.
 */

import { useCallback, useEffect, useRef, useState } from 'react';
import {
  connectTelemetrySocket,
  getEmptyTelemetry,
  getMazeMockData,
  getMockTelemetrySnapshot,
  sendStartRaceCommand,
} from '../services/telemetryService';

const RECONNECT_DELAY_MS = 2000;
const TICK_MS = 800;
const TELEMETRY_MODE = import.meta.env.VITE_TELEMETRY_MODE || 'mock';

function useLiveTelemetry() {
  const [data, setData] = useState(getEmptyTelemetry());
  const [connected, setConnected] = useState(false);
  const socketRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const mountedRef = useRef(true);

  useEffect(() => {
    mountedRef.current = true;

    function connect() {
      socketRef.current = connectTelemetrySocket({
        onOpen: () => {
          if (mountedRef.current) {
            setConnected(true);
          }
        },
        onClose: () => {
          if (!mountedRef.current) {
            return;
          }

          setConnected(false);
          reconnectTimeoutRef.current = setTimeout(connect, RECONNECT_DELAY_MS);
        },
        onTelemetry: (telemetry) => {
          if (mountedRef.current) {
            setData(telemetry);
          }
        },
      });
    }

    connect();

    return () => {
      mountedRef.current = false;
      clearTimeout(reconnectTimeoutRef.current);
      socketRef.current?.close();
    };
  }, []);

  const start = useCallback(() => {
    sendStartRaceCommand(socketRef.current);
  }, []);

  const reset = useCallback(() => {
    setData(getEmptyTelemetry());
  }, []);

  return { data, connected, start, reset, mode: 'live' };
}

function useMockTelemetry(mazeSize) {
  const [step, setStep] = useState(0);
  const [status, setStatus] = useState('waiting');
  const intervalRef = useRef(null);

  useEffect(() => {
    clearInterval(intervalRef.current);
    setStep(0);
    setStatus('waiting');
  }, [mazeSize]);

  const totalSteps = getMazeMockData(mazeSize).path.length;
  const snapshot = getMockTelemetrySnapshot(step, mazeSize);
  const data = {
    ...snapshot,
    position: status === 'waiting' ? snapshot.start : snapshot.position,
    visitedPath: status === 'waiting' ? [snapshot.start] : snapshot.visitedPath,
    status,
    elapsedSeconds: status === 'waiting' ? 0 : snapshot.elapsedSeconds,
    batteryPercent: status === 'waiting' ? 100 : snapshot.batteryPercent,
    speedMps: status === 'waiting' ? 0 : snapshot.speedMps,
  };

  useEffect(() => {
    if (status !== 'running') {
      clearInterval(intervalRef.current);
      return undefined;
    }

    intervalRef.current = setInterval(() => {
      setStep((current) => {
        if (current >= totalSteps - 1) {
          clearInterval(intervalRef.current);
          setStatus('success');
          return current;
        }

        const next = current + 1;
        if (next >= totalSteps - 1) {
          clearInterval(intervalRef.current);
          setStatus('success');
        }
        return next;
      });
    }, TICK_MS);

    return () => clearInterval(intervalRef.current);
  }, [status, totalSteps]);

  const start = useCallback(() => {
    if (status === 'running') {
      return;
    }

    setStep(0);
    setStatus('running');
  }, [status]);

  const reset = useCallback(() => {
    clearInterval(intervalRef.current);
    setStep(0);
    setStatus('waiting');
  }, []);

  return { data, connected: true, start, reset, mode: 'mock' };
}

export function useTelemetryData(mazeSize = 8) {
  if (TELEMETRY_MODE === 'live') {
    return useLiveTelemetry();
  }

  return useMockTelemetry(mazeSize);
}
