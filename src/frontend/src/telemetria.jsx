import { useEffect, useState } from 'react';

export default function Telemetry() {
  const [telemetry, setTelemetry] = useState({
    posX: 0,
    posY: 0,
    bateria: 0,
    sensorEsq: 0
  });
  const [status, setStatus] = useState('Desconectado');

  useEffect(() => {
    const ws = new WebSocket('ws://localhost:3001');

    ws.onopen = () => setStatus('Conectado ao Broker');
    ws.onclose = () => setStatus('Desconectado');
    
    ws.onmessage = (event) => {
      try {
        const data = JSON.parse(event.data);
        setTelemetry(data);
      } catch (error) {
        console.error("Erro ao processar telemetria:", error);
      }
    };

    return () => ws.close();
  }, []);

  return (
    <div style={{ padding: '20px', fontFamily: 'sans-serif' }}>
      <h2>Telemetria Micromouse</h2>
      <p>Status: <strong>{status}</strong></p>
      
      <div style={{ display: 'flex', gap: '20px', marginTop: '20px' }}>
        <div style={{ border: '1px solid #ccc', padding: '15px', borderRadius: '8px' }}>
          <h3>Posição no Labirinto</h3>
          <p>X: {telemetry.posX} | Y: {telemetry.posY}</p>
        </div>
        
        <div style={{ border: '1px solid #ccc', padding: '15px', borderRadius: '8px' }}>
          <h3>Sensores e Bateria</h3>
          <p>Bateria: {telemetry.bateria}V</p>
          <p>Sensor Esquerdo: {telemetry.sensorEsq}</p>
        </div>
      </div>
    </div>
  );
}