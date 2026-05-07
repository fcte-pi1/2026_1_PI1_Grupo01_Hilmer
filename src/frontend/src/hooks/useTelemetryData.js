// MOCK HOOK — simula dados de telemetria em tempo real.
// TODO: substituir por integração com WebSocket ou polling HTTP do backend.

import { useCallback, useEffect, useRef, useState } from 'react';
import { getMockTelemetrySnapshot, MOCK_MOUSE_PATH } from '../services/telemetryService';

const TICK_MS = 800; // intervalo entre movimentos do mouse (mock)

export function useTelemetryData() {
  const [step, setStep] = useState(0);
  const [running, setRunning] = useState(false);
  const intervalRef = useRef(null);

  const totalSteps = MOCK_MOUSE_PATH.length;
  const data = getMockTelemetrySnapshot(step);

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
