// Hook de telemetria ao vivo: conecta no backend (broker WebSocket) e
// mantém o estado sincronizado com os dados retransmitidos da ESP32.

import { useCallback, useEffect, useRef, useState } from 'react';
import { connectTelemetrySocket, getEmptyTelemetry, sendStartRaceCommand } from '../services/telemetryService';

const RECONNECT_DELAY_MS = 2000;

export function useTelemetryData() {
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
          if (mountedRef.current) setConnected(true);
        },
        onClose: () => {
          if (!mountedRef.current) return;
          setConnected(false);
          reconnectTimeoutRef.current = setTimeout(connect, RECONNECT_DELAY_MS);
        },
        onTelemetry: (telemetry) => {
          if (mountedRef.current) setData(telemetry);
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

  return { data, connected, start, reset };
}
