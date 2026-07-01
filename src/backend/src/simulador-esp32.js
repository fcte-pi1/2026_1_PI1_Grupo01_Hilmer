// Simula o WebSocket server da ESP32 para testar o pipeline site <-> backend
// <-> robô sem precisar do hardware físico. Payload segue o formato consumido
// pelo dashboard ao vivo e também inclui campos compatíveis com TELEMETRIA.

import { WebSocketServer } from 'ws';

const PORTA = 8080;
const MAZE_SIZE = 16;
const MAP_SIZE = MAZE_SIZE * 2 + 1;
const CENTER = Math.floor(MAZE_SIZE / 2) - 1;

const wss = new WebSocketServer({ port: PORTA });

console.log(`[simulador] ESP32 simulada rodando em ws://127.0.0.1:${PORTA}`);

function celulaParaMapa(r, c) {
  return [r * 2 + 1, c * 2 + 1];
}

function gerarCaminho() {
  const path = [];
  let r = 0;
  let c = 0;

  path.push([r, c]);
  while (r < CENTER) {
    r += 1;
    path.push([r, c]);
  }
  while (c < CENTER) {
    c += 1;
    path.push([r, c]);
  }

  return path;
}

const caminho = gerarCaminho();

wss.on('connection', (ws) => {
  console.log('[simulador] Backend conectado.');

  let passo = 0;
  let rodando = false;
  let numTentativa = Math.floor(Math.random() * 10000) + 1;
  let tempoInicial = Date.now();

  const mapa = Array.from({ length: MAP_SIZE }, (_, i) =>
    Array.from({ length: MAP_SIZE }, (_, j) =>
      i === 0 || i === MAP_SIZE - 1 || j === 0 || j === MAP_SIZE - 1 ? 1 : 2,
    ),
  );

  function marcarVisitado(r, c) {
    const [mr, mc] = celulaParaMapa(r, c);
    mapa[mr][mc] = 0;
  }

  function marcarSegmento(a, b) {
    const [ar, ac] = celulaParaMapa(a[0], a[1]);
    const [br, bc] = celulaParaMapa(b[0], b[1]);
    mapa[(ar + br) / 2][(ac + bc) / 2] = 0;
  }

  function buildTelemetry(status) {
    const atual = caminho[passo] ?? caminho[0];
    const visitedPath = caminho.slice(0, passo + 1).map(([r, c]) => celulaParaMapa(r, c));

    return {
      numTentativa,
      tempoColeta: new Date().toISOString(),
      tensaoRecente: 7.2,
      correnteRecente: 1.1,
      posHRecente: atual[1],
      posVRecente: atual[0],
      velocidadeAtual: status === 'running' ? 0.55 : 0,
      bateriaAtual: Math.max(100 - passo * 3, 20),
      tensaoAtual: 7.4,
      sensorCor: '#FFFFFF',
      sensorEsquerda: 240,
      sensorDireita: 180,
      sensorFrontal: status === 'success' ? 300 : 210,
      tipoLabirinto: '16x16',
      mazeSize: MAZE_SIZE,
      position: celulaParaMapa(atual[0], atual[1]),
      start: celulaParaMapa(0, 0),
      goal: celulaParaMapa(CENTER, CENTER),
      visitedPath,
      status,
      elapsedSeconds: (Date.now() - tempoInicial) / 1000,
      batteryPercent: Math.max(100 - passo * 3, 20),
      speedMps: status === 'running' ? 0.55 : 0,
      mapa,
    };
  }

  const intervalo = setInterval(() => {
    if (!rodando) {
      ws.send(JSON.stringify(buildTelemetry('waiting')));
      return;
    }

    const atual = caminho[passo];
    marcarVisitado(atual[0], atual[1]);

    if (passo > 0) {
      marcarSegmento(caminho[passo - 1], atual);
    }

    const chegouAoCentro = passo === caminho.length - 1;
    const status = chegouAoCentro ? 'success' : 'running';
    ws.send(JSON.stringify(buildTelemetry(status)));

    if (chegouAoCentro) {
      rodando = false;
      return;
    }

    passo += 1;
  }, 300);

  ws.on('message', (data) => {
    const mensagem = data.toString().toUpperCase();

    if (mensagem.includes('INICIAR_CORRIDA')) {
      console.log('[simulador] Comando INICIAR_CORRIDA recebido, iniciando corrida simulada.');
      passo = 0;
      rodando = true;
      tempoInicial = Date.now();
      numTentativa = Math.floor(Math.random() * 10000) + 1;
    }
  });

  ws.on('close', () => {
    console.log('[simulador] Backend desconectou.');
    clearInterval(intervalo);
  });
});
