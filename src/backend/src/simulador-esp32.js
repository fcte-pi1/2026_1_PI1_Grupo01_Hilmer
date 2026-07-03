// Simula o WebSocket server da ESP32 (firmware micromouse_encoders.ino)
// para testar o pipeline site <-> backend <-> robô sem precisar do hardware
// físico. Protocolo e formato de payload espelham o firmware real:
// comandos JSON {type:"START", mazeSize}/{type:"START_RUN"}/{type:"STOP"},
// telemetria com os mesmos campos de preencherPayloadWeb().
//
// nova alt: matriz do mapa agora é mazeSize+2 (não mais mazeSize*2+1) —
// cada célula real vira 1 posição na matriz, com 1 camada de parede fixa
// ao redor. Bateria/corrente saem null (igual ao firmware real sem
// sensor instalado); velocidade é calculada a partir do avanço
// simulado, não mais um valor fixo.

import { WebSocketServer } from 'ws';

const PORTA = 8080;
const CELL_SIZE_CM = 18;
const TICK_MS = 300;

const wss = new WebSocketServer({ port: PORTA });

console.log(`[simulador] ESP32 simulada rodando em ws://127.0.0.1:${PORTA}`);

function mazeSizeValido(value) {
  return value === 4 || value === 8 || value === 16;
}

// nova alt: 1 célula real = 1 posição na matriz + borda fixa de 1 célula
// ao redor (mazeSize+2 no total) — ver comentário do MAX_MAP_SIZE no firmware.
function celulaParaMapa(r, c) {
  return [r + 1, c + 1];
}

function gerarCaminho(mazeSize) {
  const center = Math.floor(mazeSize / 2) - 1;
  const path = [];
  let r = 0;
  let c = 0;

  path.push([r, c]);
  while (r < center) {
    r += 1;
    path.push([r, c]);
  }
  while (c < center) {
    c += 1;
    path.push([r, c]);
  }

  return path;
}

wss.on('connection', (ws) => {
  console.log('[simulador] Backend conectado.');

  let mazeSize = 16;
  let mapSize = mazeSize + 2;
  let caminhoIda = gerarCaminho(mazeSize);
  let mapa = criarMapaVazio(mapSize);
  let numTentativa = Math.floor(Math.random() * 9000) + 1000;
  let tempoInicial = Date.now();
  let passo = 0;
  let velocidadeAtualMps = 0;
  // Estados espelhando o firmware: AGUARDANDO_INICIO, MAPEANDO, MAPEAMENTO_CONCLUIDO, CORRIDA, FINALIZADO
  let fase = 'AGUARDANDO_INICIO';

  function criarMapaVazio(tamanho) {
    return Array.from({ length: tamanho }, (_, i) =>
      Array.from({ length: tamanho }, (_, j) =>
        i === 0 || i === tamanho - 1 || j === 0 || j === tamanho - 1 ? 1 : 2,
      ),
    );
  }

  // nova alt: só marca a própria célula visitada (0) — sem resolução dupla
  // não existe mais uma posição "entre" duas células pra marcar corredor.
  function marcarVisitado(r, c) {
    const [mr, mc] = celulaParaMapa(r, c);
    mapa[mr][mc] = 0;
  }

  function reiniciarTentativa(novoMazeSize) {
    mazeSize = novoMazeSize;
    mapSize = mazeSize + 2;
    caminhoIda = gerarCaminho(mazeSize);
    mapa = criarMapaVazio(mapSize);
    numTentativa += 1;
    tempoInicial = Date.now();
    passo = 0;
    velocidadeAtualMps = 0;
    fase = 'MAPEANDO';
    marcarVisitado(0, 0);
  }

  function caminhoAtual() {
    return fase === 'CORRIDA' ? [...caminhoIda].reverse() : caminhoIda;
  }

  function posicaoAtual() {
    const caminho = caminhoAtual();
    return caminho[Math.min(passo, caminho.length - 1)];
  }

  function buildTelemetry() {
    const caminho = caminhoAtual();
    const atual = posicaoAtual();
    const visitedPath = caminho.slice(0, passo + 1).map(([r, c]) => celulaParaMapa(r, c));
    const elapsedSeconds = (Date.now() - tempoInicial) / 1000;
    const desafioCumprido = fase === 'FINALIZADO';
    const velocidadeMedia = elapsedSeconds > 0 ? (passo * CELL_SIZE_CM) / 100 / elapsedSeconds : 0;

    let status = 'running';
    if (desafioCumprido) status = 'success';
    else if (fase === 'AGUARDANDO_INICIO') status = 'waiting_start';
    else if (fase === 'MAPEAMENTO_CONCLUIDO') status = 'waiting_run';

    return {
      numTentativa,
      tempoColeta: new Date().toISOString(),
      // nova alt: sem sensor de tensão/corrente instalado — null, igual
      // ao firmware real (ver ADC_BATTERY_PIN/ADC_CURRENT_PIN no .ino).
      tensaoRecente: null,
      correnteRecente: null,
      posHRecente: atual[1],
      posVRecente: atual[0],
      velocidadeAtual: velocidadeAtualMps,
      bateriaAtual: null,
      tensaoAtual: null,
      sensorCor: '#000000',
      sensorEsquerda: 0,
      sensorDireita: 0,
      sensorFrontal: 0,
      mazeSize,
      mapSize,
      tipoLabirinto: `${mazeSize}x${mazeSize}`,
      estadoRobo: fase === 'AGUARDANDO_INICIO' ? 1 : fase === 'FINALIZADO' ? 7 : 2,
      modoOperacao: fase === 'CORRIDA' || fase === 'FINALIZADO' ? 'CORRIDA' : 'MAPEAMENTO',
      aguardandoInicio: fase === 'AGUARDANDO_INICIO',
      aguardandoCorrida: fase === 'MAPEAMENTO_CONCLUIDO',
      travado: false,
      desafioCumprido: desafioCumprido ? 'SIM' : 'NAO',
      status,
      elapsedSeconds,
      batteryPercent: null,
      speedMps: velocidadeAtualMps,
      position: celulaParaMapa(atual[0], atual[1]),
      start: celulaParaMapa(0, 0),
      goal: celulaParaMapa(...caminhoIda[caminhoIda.length - 1]),
      visitedPath,
      trajetoAtual: {
        numTentativa,
        passo: passo + 1,
        pos_h: atual[1],
        pos_v: atual[0],
        direcao: 'NORTE',
      },
      mapa,
      historico: {
        percentualBateria: null,
        velocidadeMedia,
        tempoConclusao: desafioCumprido ? new Date().toISOString() : '',
        desafioCumprido: desafioCumprido ? 'SIM' : 'NAO',
        correnteEletrica: null,
        tensaoEletrica: null,
        tipoLabirinto: `${mazeSize}x${mazeSize}`,
        mazeSize,
      },
    };
  }

  function enviarTelemetria() {
    ws.send(JSON.stringify(buildTelemetry()));
  }

  function responderComando(command, extra = {}) {
    ws.send(JSON.stringify({ type: 'ACK', command, status: 'queued', ...extra }));
  }

  const intervalo = setInterval(() => {
    if (fase === 'MAPEANDO' || fase === 'CORRIDA') {
      const caminho = caminhoAtual();
      const atual = caminho[passo];
      marcarVisitado(atual[0], atual[1]);

      // nova alt: velocidade "real" simulada — distância de uma célula
      // dividida pelo tempo entre dois ticks, em vez de um valor fixo.
      velocidadeAtualMps = CELL_SIZE_CM / 100 / (TICK_MS / 1000);

      const chegouAoFim = passo === caminho.length - 1;

      if (chegouAoFim) {
        velocidadeAtualMps = 0;
        if (fase === 'MAPEANDO') {
          fase = 'MAPEAMENTO_CONCLUIDO';
          console.log('[simulador] Mapeamento concluído, aguardando START_RUN.');
        } else {
          fase = 'FINALIZADO';
          console.log('[simulador] Corrida final concluída!');
        }
      } else {
        passo += 1;
      }
    } else {
      velocidadeAtualMps = 0;
    }

    enviarTelemetria();
  }, TICK_MS);

  ws.on('message', (data) => {
    let comando;
    try {
      comando = JSON.parse(data.toString());
    } catch {
      // Compatibilidade com o protocolo antigo (texto puro "INICIAR_CORRIDA").
      if (data.toString().toUpperCase().includes('INICIAR_CORRIDA')) {
        fase = 'CORRIDA';
        passo = 0;
        tempoInicial = Date.now();
      }
      return;
    }

    const type = String(comando?.type ?? '').toUpperCase();

    if (type === 'START') {
      const novoMazeSize = Number(comando.mazeSize);
      if (!mazeSizeValido(novoMazeSize)) {
        ws.send(JSON.stringify({ type: 'ERROR', message: 'mazeSize invalido. Use 4, 8 ou 16.', receivedMazeSize: novoMazeSize }));
        return;
      }
      reiniciarTentativa(novoMazeSize);
      responderComando('START', { mazeSize: novoMazeSize });
      console.log(`[simulador] START recebido. mazeSize=${novoMazeSize}`);
      return;
    }

    if (type === 'START_RUN') {
      if (fase === 'MAPEAMENTO_CONCLUIDO') {
        fase = 'CORRIDA';
        passo = 0;
        tempoInicial = Date.now();
        console.log('[simulador] START_RUN recebido, iniciando corrida final.');
      }
      responderComando('START_RUN');
      return;
    }

    if (type === 'STOP') {
      fase = 'AGUARDANDO_INICIO';
      passo = 0;
      velocidadeAtualMps = 0;
      responderComando('STOP');
      console.log('[simulador] STOP recebido.');
      return;
    }
  });

  ws.on('close', () => {
    console.log('[simulador] Backend desconectou.');
    clearInterval(intervalo);
  });

  enviarTelemetria();
});
