import { WebSocketServer } from 'ws';

const PORTA = 8080;
const wss = new WebSocketServer({ port: PORTA });

console.log(`Simulador da ESP32-C3 rodando em ws://127.0.0.1:${PORTA}`);

wss.on('connection', (ws) => {
  console.log('Conexão recebida! (O Backend Node.js conectou no simulador)');

  const intervalo = setInterval(() => {
    const fakeTelemetry = {
      posX: Math.floor(Math.random() * 16), // Posição X no labirinto (0 a 15)
      posY: Math.floor(Math.random() * 16), // Posição Y no labirinto (0 a 15)
      bateria: (7.0 + Math.random() * 1.4).toFixed(2), // Bateria oscilando entre 7.0V e 8.4V
      sensorEsq: Math.floor(Math.random() * 4095) // Leitura analógica aleatória
    };
    
    ws.send(JSON.stringify(fakeTelemetry));
  }, 100);

  ws.on('close', () => {
    console.log('Backend desconectou do simulador.');
    clearInterval(intervalo);
  });
});