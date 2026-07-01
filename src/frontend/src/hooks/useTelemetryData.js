/**
 * useTelemetryData.js
 *
 * Hook de telemetria. Comportamento por modo:
 *
 *   'live'  → WebSocket real (ESP32 via backend). Usado em produção/hardware.
 *   'mock'  → Simulação local com dados estáticos. Usado sem hardware.
 *
 * O modo é controlado pela variável de ambiente VITE_TELEMETRY_MODE.
 * Se não definida, usa 'mock' por padrão (seguro para desenvolvimento).
 *
 * TODO: remover fallback mock quando a ESP32 estiver integrada de forma estável.
 */

import { useCallback, useEffect, useRef, useState } from 'react';
import { getMockTelemetrySnapshot, getMazeMockData } from '../services/telemetryService';

const TICK_MS = 800;
const WS_URL = import.meta.env.VITE_WS_URL || 'ws://localhost:3001';
const TELEMETRY_MODE = import.meta.env.VITE_TELEMETRY_MODE || 'mock'; // 'live' | 'mock'

// ─── Modo Mock (sem hardware) ─────────────────────────────────────────────────

function useMockTelemetry(mazeSize) {
  const [step, setStep] = useState(0);
  const [running, setRunning] = useState(false);
  const intervalRef = useRef(null);

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

  return { data, running, start, reset, mode: 'mock' };
}

// ─── Modo Live (WebSocket real) ───────────────────────────────────────────────

function useLiveTelemetry(mazeSize) {
  const [data, setData] = useState(getMockTelemetrySnapshot(0, mazeSize)); // estado inicial neutro
  const [running, setRunning] = useState(false);
  const [wsStatus, setWsStatus] = useState('disconnected'); // 'connected' | 'disconnected' | 'error'
  const wsRef = useRef(null);

  useEffect(() => {
    const ws = new WebSocket(WS_URL);
    wsRef.current = ws;

    ws.onopen = () => {
      setWsStatus('connected');
      setRunning(true);
    };

    ws.onmessage = (event) => {
      try {
        const parsed = JSON.parse(event.data);
        // TODO: validar schema do payload da ESP32 aqui com Zod ou checagem manual
        // Esperado: { position, grid, goal, start, status, elapsedSeconds, batteryPercent, speedMps }
        setData(parsed);
      } catch {
        console.warn('[useTelemetryData] Payload inválido recebido via WebSocket:', event.data);
      }
    };

    ws.onclose = () => {
      setWsStatus('disconnected');
      setRunning(false);
    };

    ws.onerror = () => {
      setWsStatus('error');
    };

    return () => ws.close();
  }, []);

  // No modo live, start/reset controlam reconexão
  const start = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN) return;
    // TODO: implementar reconexão manual se necessário
  }, []);

  const reset = useCallback(() => {
    wsRef.current?.close();
    setRunning(false);
    setData(getMockTelemetrySnapshot(0, mazeSize));
  }, [mazeSize]);

  return { data, running, start, reset, mode: 'live', wsStatus };
}

// ─── Export unificado ─────────────────────────────────────────────────────────

export function useTelemetryData(mazeSize = 10) {
  const mock = useMockTelemetry(mazeSize);
  const live = useLiveTelemetry(mazeSize);

  return TELEMETRY_MODE === 'live' ? live : mock;
}