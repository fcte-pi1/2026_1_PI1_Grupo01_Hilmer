import { WebSocketServer } from 'ws';

// Porta do simulador — deve coincidir com ESP32_WS_URL no .env (ex: ws://127.0.0.1:8080)
const PORTA = 8080;
const wss = new WebSocketServer({ port: PORTA });

console.log(`Simulador da ESP32-C3 rodando em ws://127.0.0.1:${PORTA}`);

function randomBetween(min, max, decimals = 2) {
  const v = Math.random() * (max - min) + min;
  return Number(v.toFixed(decimals));
}

function pick(arr) {
  return arr[Math.floor(Math.random() * arr.length)];
}

wss.on('connection', (ws) => {
  console.log('Conexão recebida! (O Backend Node.js conectou no simulador)');

  // Simula um identificador de tentativa que poderia ser gerado pelo backend/ESP
  let numTentativa = Math.floor(Math.random() * 10000) + 1;

  const intervalo = setInterval(() => {
    const payload = {
      numTentativa,
      tempoColeta: new Date().toISOString(),
      // Tensões / correntes simuladas
      tensaoRecente: randomBetween(6.8, 7.6, 2),
      correnteRecente: randomBetween(0.8, 2.5, 2),
      // Posição do robô no labirinto
      posHRecente: Math.floor(Math.random() * 16),
      posVRecente: Math.floor(Math.random() * 16),
      velocidadeAtual: randomBetween(0.0, 1.0, 2),
      bateriaAtual: randomBetween(20, 100, 1),
      tensaoAtual: randomBetween(6.8, 7.6, 2),
      // Sensores
      sensorCor: pick(['#ffffff', '#ff0000']),
      sensorEsquerda: randomBetween(0, 300, 1),
      sensorDireita: randomBetween(0, 300, 1),
      sensorFrontal: randomBetween(0, 300, 1),
      // Opcional: mapa ou resumo do labirinto
      tipoLabirinto: pick(['16x16', '8x8', '4x4'])
    };

    ws.send(JSON.stringify(payload));
  }, 200);

  ws.on('close', () => {
    console.log('Backend desconectou do simulador.');
    clearInterval(intervalo);
  });
});