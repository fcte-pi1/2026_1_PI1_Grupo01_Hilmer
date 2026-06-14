/**
 * apiService.js — Frontend
 *
 * Camada centralizada de comunicação HTTP com o backend.
 * Mapeada exatamente às tabelas HISTORICO, TELEMETRIA e TRAJETO.
 */

const BASE_URL = import.meta.env.VITE_API_URL || '';

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

export async function criarHistorico(payload) {
  return request('/api/historico', { method: 'POST', body: JSON.stringify(payload) });
}

export async function listarHistorico() {
  return request('/api/historico');
}

export async function buscarHistorico(numTentativa) {
  return request(`/api/historico/${numTentativa}`);
}

export async function analisarTentativa(numTentativa) {
  return request(`/api/historico/${numTentativa}/analise`);
}

// ─── TELEMETRIA ───────────────────────────────────────────────────────────────

export async function listarTelemetria(numTentativa) {
  return request(`/api/telemetria/${numTentativa}`);
}

// ─── TRAJETO ──────────────────────────────────────────────────────────────────

export async function listarTrajeto(numTentativa) {
  return request(`/api/trajeto/${numTentativa}`);
}

export async function criarPassoTrajeto(payload) {
  return request('/api/trajeto', { method: 'POST', body: JSON.stringify(payload) });
}

// ─── Mouse / FloodFill ────────────────────────────────────────────────────────

export async function getNextMove(currentPosition, mazeMatrix) {
  return request('/api/mouse/next-move', {
    method: 'POST',
    body: JSON.stringify({ currentPosition, mazeMatrix }),
  });
}

export async function getHealth() {
  return request('/api/health');
}

export async function reconectarEsp32() {
  return request('/api/esp32/reconnect', { method: 'POST' });
}
