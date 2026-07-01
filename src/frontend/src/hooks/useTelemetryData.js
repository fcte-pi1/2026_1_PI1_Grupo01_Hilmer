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
  sendStartMappingCommand,
  sendStartRaceCommand,
  sendStopCommand,
} from '../services/telemetryService';
import { getStatusEsp32, reconectarEsp32 } from '../services/apiService';

const RECONNECT_DELAY_MS = 2000;
const TICK_MS = 800;
const ESP32_STATUS_POLL_MS = 4000;
const TELEMETRY_MODE = import.meta.env.VITE_TELEMETRY_MODE || 'mock';

function useLiveTelemetry(mazeSize) {
  const [data, setData] = useState(getEmptyTelemetry());
  const [connected, setConnected] = useState(false);
  // `connected` só diz se o navegador está falando com o backend (broker).
  // `esp32Connected` é o estado real do robô, consultado à parte, pois o
  // broker fica de pé mesmo com a ESP32 desligada/fora de alcance.
  const [esp32Connected, setEsp32Connected] = useState(false);
  const socketRef = useRef(null);
  const reconnectTimeoutRef = useRef(null);
  const mountedRef = useRef(true);
  // Garante que o START (com o tamanho do labirinto escolhido em NewAttempt)
  // é enviado só uma vez por montagem do Dashboard — não a cada reconexão do
  // broker/ESP32, pra não reiniciar o mapeamento/corrida do robô à toa.
  const mappingStartedRef = useRef(false);

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

  useEffect(() => {
    let cancelled = false;

    async function pollEsp32Status() {
      try {
        const response = await getStatusEsp32();
        if (!cancelled) {
          setEsp32Connected(Boolean(response?.data?.connected));
        }
      } catch {
        if (!cancelled) {
          setEsp32Connected(false);
        }
      }
    }

    pollEsp32Status();
    const intervalId = setInterval(pollEsp32Status, ESP32_STATUS_POLL_MS);

    return () => {
      cancelled = true;
      clearInterval(intervalId);
    };
  }, []);

  // Assim que o broker E a ESP32 estiverem prontos, manda o comando START
  // com o tamanho do labirinto escolhido — é isso que faz o robô sair de
  // AGUARDANDO_INICIO e começar o mapeamento de verdade.
  useEffect(() => {
    if (!connected || !esp32Connected || !mazeSize || mappingStartedRef.current) {
      return;
    }

    const sent = sendStartMappingCommand(socketRef.current, mazeSize);
    if (sent) {
      mappingStartedRef.current = true;
    }
  }, [connected, esp32Connected, mazeSize]);

  const start = useCallback(() => {
    sendStartRaceCommand(socketRef.current);
  }, []);

  const stop = useCallback(() => {
    sendStopCommand(socketRef.current);
  }, []);

  const reset = useCallback(() => {
    setData(getEmptyTelemetry());
  }, []);

  const reconnectEsp32 = useCallback(async () => {
    try {
      const response = await reconectarEsp32();
      if (mountedRef.current) {
        setEsp32Connected(Boolean(response?.data?.connected));
      }
    } catch (error) {
      console.error('[telemetria] Falha ao reconectar a ESP32:', error);
    }
  }, []);

  return { data, connected, esp32Connected, start, stop, reset, reconnectEsp32, mode: 'live' };
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

  // No mock não existe robô de verdade pra checar/parar; mantém o mesmo
  // formato de retorno do modo live pra não exigir lógica condicional nos
  // componentes.
  const reconnectEsp32 = useCallback(async () => {}, []);
  const stop = useCallback(() => {
    clearInterval(intervalRef.current);
    setStatus('waiting');
  }, []);

  return { data, connected: true, esp32Connected: true, start, stop, reset, reconnectEsp32, mode: 'mock' };
}

export function useTelemetryData(mazeSize = 8) {
  if (TELEMETRY_MODE === 'live') {
    return useLiveTelemetry(mazeSize);
  }

  return useMockTelemetry(mazeSize);
}
