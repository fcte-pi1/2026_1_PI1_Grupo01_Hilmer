/**
 * Fixture reutilizável para testes de GET /analise (labirinto 4x4).
 */

export const historico4x4 = { tipolabirinto: '4x4' };

export const trajeto4x4 = [
  { passo: 1, pos_h: 0, pos_v: 0, direcao: 'LESTE' },
  { passo: 2, pos_h: 1, pos_v: 0, direcao: 'SUL' },
  { passo: 3, pos_h: 1, pos_v: 1, direcao: 'NORTE' },
  { passo: 4, pos_h: 1, pos_v: 0, direcao: 'OESTE' },
];

export const telemetria4x4 = [
  {
    posHRecente: 0,
    posVRecente: 0,
    tempoColeta: '2026-01-01T00:00:00.000Z',
    sensorFrontal: 300,
    sensorEsquerda: 300,
    sensorDireita: 50,
  },
];

export function buildHistoricoPayload4x4(overrides = {}) {
  return {
    percentualBateria: 88,
    velocidadeMedia: 0.5,
    tempoConclusao: new Date().toISOString(),
    desafioCumprido: 'SIM',
    correnteEletrica: 1.1,
    tensaoEletrica: 7.4,
    tipoLabirinto: '4x4',
    ...overrides,
  };
}

export function buildTelemetriaHttpPayload(numTentativa, entry = telemetria4x4[0]) {
  return {
    numTentativa,
    tempoColeta: entry.tempoColeta ?? new Date().toISOString(),
    tensaoRecente: 7.2,
    correnteRecente: 1.0,
    posHRecente: entry.posHRecente,
    posVRecente: entry.posVRecente,
    velocidadeAtual: 0.45,
    bateriaAtual: 85,
    tensaoAtual: 7.3,
    sensorEsquerda: entry.sensorEsquerda,
    sensorDireita: entry.sensorDireita,
    sensorFrontal: entry.sensorFrontal,
  };
}

/**
 * Cria tentativa completa (HISTORICO + TRAJETO + TELEMETRIA) via HTTP.
 * @param {string} baseUrl ex.: http://127.0.0.1:3001
 * @returns {Promise<number>} numTentativa criado
 */
export async function seedAnalysisAttemptViaHttp(baseUrl = 'http://127.0.0.1:3001') {
  const historicoResponse = await fetch(`${baseUrl}/api/historico`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(buildHistoricoPayload4x4()),
  });

  if (!historicoResponse.ok) {
    const errorBody = await historicoResponse.text();
    throw new Error(`Falha ao criar HISTORICO: ${historicoResponse.status} — ${errorBody}`);
  }

  const historicoData = await historicoResponse.json();
  const numTentativa = historicoData.data.numtentativa;

  for (const step of trajeto4x4) {
    const trajetoResponse = await fetch(`${baseUrl}/api/trajeto`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ numTentativa, ...step }),
    });

    if (!trajetoResponse.ok) {
      throw new Error(`Falha ao criar TRAJETO passo ${step.passo}: ${trajetoResponse.status}`);
    }
  }

  const telemetriaResponse = await fetch(`${baseUrl}/api/telemetria`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(buildTelemetriaHttpPayload(numTentativa)),
  });

  if (!telemetriaResponse.ok) {
    throw new Error(`Falha ao criar TELEMETRIA: ${telemetriaResponse.status}`);
  }

  return numTentativa;
}
