// Simula o WebSocket server da ESP32 para testar o pipeline site <-> backend
// <-> robô sem precisar do hardware físico. Payload segue o mesmo formato
// enviado pelo firmware real (ver preencherPayloadWeb no .cpp do micromouse).
//
// Uso: rode este script e aponte o backend para ele via
// ESP32_WS_URL=ws://127.0.0.1:8080 (em vez do IP padrão da ESP32).
import { WebSocketServer } from 'ws';

const PORTA = 8080;
const MAZE_SIZE = 16;
const MAP_SIZE = MAZE_SIZE * 2 + 1;
const CENTER = Math.floor(MAZE_SIZE / 2) - 1; // igual ao firmware: goal = (mid-1, mid-1)

const wss = new WebSocketServer({ port: PORTA });
const tempoInicial = Date.now();

console.log(`[simulador] ESP32 simulada rodando em ws://127.0.0.1:${PORTA}`);

function celulaParaMapa(r, c) {
  return [r * 2 + 1, c * 2 + 1];
}

// Caminho simples em "L": desce até a linha do centro, depois anda até a coluna do centro.
function gerarCaminho() {
  const path = [];
  let r = 0;
  let c = 0;
  path.push([r, c]);
  while (r < CENTER) { r++; path.push([r, c]); }
  while (c < CENTER) { c++; path.push([r, c]); }
  return path;
}

const caminho = gerarCaminho();

wss.on('connection', (ws) => {
  console.log('[simulador] Backend conectado.');

  // Igual ao firmware real: fica parado em "waiting" até receber
  // INICIAR_CORRIDA pelo WebSocket, só então começa a se mover.
  let passo = 0;
  let rodando = false;
  const mapa = Array.from({ length: MAP_SIZE }, (_, i) =>
    Array.from({ length: MAP_SIZE }, (_, j) =>
      (i === 0 || i === MAP_SIZE - 1 || j === 0 || j === MAP_SIZE - 1) ? 1 : 2
    )
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

  const intervalo = setInterval(() => {
    if (!rodando) {
      const telemetria = {
        position: celulaParaMapa(0, 0),
        start: celulaParaMapa(0, 0),
        goal: celulaParaMapa(CENTER, CENTER),
        visitedPath: [celulaParaMapa(0, 0)],
        status: 'waiting',
        elapsedSeconds: 0,
        batteryPercent: 100,
        speedMps: 0,
        mapa,
      };

      ws.send(JSON.stringify(telemetria));
      return;
    }

    const atual = caminho[passo];
    marcarVisitado(atual[0], atual[1]);
    if (passo > 0) marcarSegmento(caminho[passo - 1], atual);

    const chegouAoCentro = passo === caminho.length - 1;
    const visitedPath = caminho.slice(0, passo + 1).map(([r, c]) => celulaParaMapa(r, c));

    const telemetria = {
      position: celulaParaMapa(atual[0], atual[1]),
      start: celulaParaMapa(0, 0),
      goal: celulaParaMapa(CENTER, CENTER),
      visitedPath,
      status: chegouAoCentro ? 'success' : 'running',
      elapsedSeconds: (Date.now() - tempoInicial) / 1000,
      batteryPercent: Math.max(100 - passo * 3, 20),
      speedMps: 0.5 + Math.random() * 0.1,
      mapa,
    };

    ws.send(JSON.stringify(telemetria));

    if (chegouAoCentro) {
      rodando = false; // volta a esperar um novo comando, igual ao robô real
    } else {
      passo++;
    }
  }, 300);

  ws.on('message', (data) => {
    const mensagem = data.toString().toUpperCase();
    if (mensagem.includes('INICIAR_CORRIDA')) {
      console.log('[simulador] Comando INICIAR_CORRIDA recebido, iniciando corrida simulada.');
      passo = 0;
      rodando = true;
    }
  });

  ws.on('close', () => {
    console.log('[simulador] Backend desconectou.');
    clearInterval(intervalo);
  });
});
