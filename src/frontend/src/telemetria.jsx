import { useEffect, useState } from 'react';
import Maze from './components/Maze';

export default function Telemetry() {
  const [telemetry, setTelemetry] = useState({
    tipoLabirinto: "Não definido",
    bateriaConsumo: 0,
    velocidadeMedia: 0,
    tempoConclusao: 0,
    desafioCumprido: "N",
    mapa: [] // Começa vazio até receber da ESP32
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
      
      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '20px' }}>
        {/* Bloco de Informações Gerais */}
        <div style={{ border: '1px solid #ccc', padding: '15px', borderRadius: '8px' }}>
          <h3>Métricas da Corrida</h3>
          <p>Tamanho do Labirinto: <strong>{telemetry.tipoLabirinto}</strong></p>
          <p>Velocidade Média: <strong>{telemetry.velocidadeMedia} m/s</strong></p>
          <p>Tempo de Conclusão: <strong>{telemetry.tempoConclusao}s</strong></p>
          <p>Consumo de Bateria: <strong>{telemetry.bateriaConsumo}%</strong></p>
          <p>Desafio Cumprido: 
            <span style={{ color: telemetry.desafioCumprido === 'S' ? 'green' : 'red', fontWeight: 'bold' }}>
              {telemetry.desafioCumprido}
            </span>
          </p>
        </div>

        {/* Bloco Visual do Labirinto (Grade de Matriz) */}
        <div style={{ border: '1px solid #ccc', padding: '15px', borderRadius: '8px' }}>
          <h3>Mapa do Labirinto (33x33)</h3>
          <Maze rows={33} cols={33} cell_size={10} />
        </div>
      </div>
    </div>
  );
}