/**
 * liveTrajetoBuffer.js
 *
 * Acumula passos de trajetoAtual recebidos da ESP32 durante a corrida.
 * TELEMETRIA e TRAJETO só podem ser gravados no banco após criar HISTORICO;
 * os passos ficam em memória até attemptService drenar ao persistir success
 * ou POST /api/historico.
 *
 * Chave por passo: deduplica leituras repetidas do mesmo passo, mantendo
 * a mais recente.
 */

const VALID_DIRECTIONS = new Set(['NORTE', 'SUL', 'LESTE', 'OESTE']);

/** @type {Map<number, { passo: number, pos_h: number, pos_v: number, direcao: string }>} */
let buffer = new Map();

function normalizeDirection(direcao) {
  const value = String(direcao ?? 'NORTE').toUpperCase();
  return VALID_DIRECTIONS.has(value) ? value : 'NORTE';
}

function registrar(trajetoAtual) {
  if (!trajetoAtual) {
    return;
  }

  const passo = Number(trajetoAtual.passo);
  if (!Number.isFinite(passo) || passo < 1) {
    return;
  }

  const posH = Number(trajetoAtual.pos_h ?? trajetoAtual.posH);
  const posV = Number(trajetoAtual.pos_v ?? trajetoAtual.posV);
  if (!Number.isFinite(posH) || !Number.isFinite(posV)) {
    return;
  }

  buffer.set(passo, {
    passo,
    pos_h: posH,
    pos_v: posV,
    direcao: normalizeDirection(trajetoAtual.direcao),
  });
}

function listar() {
  return Array.from(buffer.values()).sort((a, b) => a.passo - b.passo);
}

function limpar() {
  buffer.clear();
}

export default { registrar, listar, limpar };
