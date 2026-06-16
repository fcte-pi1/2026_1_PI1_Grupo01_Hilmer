// MOCK HOOK — simula dados de telemetria em tempo real.
// TODO: substituir por integração com WebSocket ou polling HTTP do backend.

import { useCallback, useEffect, useRef, useState } from 'react';
import { getMockTelemetrySnapshot, getMazeMockData } from '../services/telemetryService';

const RUN_SPEED = {
  1: 800,
  2: 320,
};

export function useTelemetryData(mazeSize = 10, run = 1) {
  const [step, setStep] = useState(0);
  const [running, setRunning] = useState(false);
  const intervalRef = useRef(null);

  useEffect(() => {
    clearInterval(intervalRef.current);
    setRunning(false);
    setStep(0);
  }, [mazeSize, run]);

  const totalSteps = getMazeMockData(mazeSize).path.length;
  const data = getMockTelemetrySnapshot(step, mazeSize, run);
  const tickMs = RUN_SPEED[run] ?? RUN_SPEED[1];

  const start = useCallback(() => {
    if (running) return;
    setStep(0);
    setRunning(true);
  }, [running]);

  const reset = useCallback(() => {
    clearInterval(intervalRef.current);
    setRunning(false);
    setStep(0);
  }, []);

  useEffect(() => {
    if (!running) return;

    intervalRef.current = setInterval(() => {
      setStep((prev) => {
        if (prev >= totalSteps - 1) {
          clearInterval(intervalRef.current);
          setRunning(false);
          return prev;
        }
        return prev + 1;
      });
    }, tickMs);

    return () => clearInterval(intervalRef.current);
  }, [running, totalSteps, tickMs]);

  return { data, running, start, reset };
}
