/**
 * apiService.js — Frontend
 *
 * Camada centralizada de comunicação HTTP com o backend.
 * Mapeada exatamente às tabelas HISTORICO, TELEMETRIA e TRAJETO.
 */

const BASE_URL = import.meta.env.VITE_API_URL || 'http://localhost:3001';

async function request(path, options = {}) {
  const res = await fetch(`${BASE_URL}${path}`, {
    headers: { 'Content-Type': 'application/json', ...options.headers },
    ...options,
  });
  const data = await res.json();
  if (!res.ok) throw new Error(data?.error || `Erro ${res.status}`);
  return data;
}

// ─── HISTORICO ────────────────────────────────────────────────────────────────

/**
 * Registra uma tentativa concluída.
 * @param {{
 *   numTentativa: number,
 *   percentualBateria: number,
 *   velocidadeMedia: number,
 *   tempoConclusao: string,
 *   desafioCumprido: 'SIM'|'NAO',
 *   correnteEletrica: number,
 *   tensaoEletrica: number,
 *   tipoLabirinto: '4x4'|'8x8'|'16x16'
 * }} payload
 */
export async function criarHistorico(payload) {
  return request('/api/historico', { method: 'POST', body: JSON.stringify(payload) });
}

export async function listarHistorico() {
  return request('/api/historico');
}

export async function buscarHistorico(numTentativa) {
  return request(`/api/historico/${numTentativa}`);
}

// ─── TELEMETRIA ───────────────────────────────────────────────────────────────

/**
 * Retorna toda a telemetria de uma tentativa (para replay/análise no frontend).
 * @param {number} numTentativa
 */
export async function listarTelemetria(numTentativa) {
  return request(`/api/telemetria/${numTentativa}`);
}

// ─── TRAJETO ──────────────────────────────────────────────────────────────────

/**
 * Retorna o trajeto completo de uma tentativa.
 * @param {number} numTentativa
 */
export async function listarTrajeto(numTentativa) {
  return request(`/api/trajeto/${numTentativa}`);
}

// ─── Mouse / FloodFill ────────────────────────────────────────────────────────

/**
 * Calcula próximo movimento via FloodFill no backend.
 * @param {{ row: number, col: number }} currentPosition
 * @param {number[][]} mazeMatrix
 */
export async function getNextMove(currentPosition, mazeMatrix) {
  return request('/api/mouse/next-move', {
    method: 'POST',
    body: JSON.stringify({ currentPosition, mazeMatrix }),
  });
}

export async function getHealth() {
  return request('/api/health');
}