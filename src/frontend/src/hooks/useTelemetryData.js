// MOCK HOOK — simula dados de telemetria em tempo real.
// TODO: substituir por integração com WebSocket ou polling HTTP do backend.

import { useCallback, useEffect, useRef, useState } from 'react';
import { getMockTelemetrySnapshot, getMazeMockData } from '../services/telemetryService';

const TICK_MS = 800; // intervalo entre movimentos do mouse (mock)

export function useTelemetryData(mazeSize = 10) {
  const [step, setStep] = useState(0);
  const [running, setRunning] = useState(false);
  const intervalRef = useRef(null);

  useEffect(() => {
    clearInterval(intervalRef.current);
    setRunning(false);
    setStep(0);
  }, [mazeSize]);

  const totalSteps = getMazeMockData(mazeSize).path.length;
  const data = getMockTelemetrySnapshot(step, mazeSize);

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
    }, TICK_MS);

    return () => clearInterval(intervalRef.current);
  }, [running, totalSteps]);

  return { data, running, start, reset };
}
