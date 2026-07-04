/**
 * liveTelemetryBuffer.js
 *
 * A ESP32 manda telemetria em tempo real, mas TELEMETRIA tem FK para
 * HISTORICO, e o numTentativa de HISTORICO só existe quando o site
 * confirma a tentativa (POST /api/historico, ao final da corrida) — o
 * numTentativa que a ESP32 manda em cada mensagem é só um id efêmero
 * dela mesma, sem relação com o id gerado pelo banco.
 *
 * Por isso os snapshots ficam em memória aqui enquanto a corrida roda
 * (attemptService chama registrar() via handleLiveTelemetry) e só são
 * gravados de fato quando attemptService persiste success ou POST
 * /api/historico chama drenar() com o numTentativa real do banco.
 *
 * Chave por tempoColeta: a ESP32 só tem resolução de segundo no
 * timestamp, e TELEMETRIA tem PK (numTentativa, tempoColeta) — usar um
 * Map deduplica automaticamente leituras do mesmo segundo, mantendo a
 * mais recente.
 */

const MAX_BUFFER_SIZE = 5000;

let buffer = new Map();

function registrar(payload) {
  const {
    tempoColeta, tensaoRecente, correnteRecente, posHRecente, posVRecente,
    velocidadeAtual, bateriaAtual, tensaoAtual,
    sensorCor = null, sensorEsquerda = null, sensorDireita = null, sensorFrontal = null,
  } = payload;

  if (!tempoColeta) {
    return;
  }

  buffer.set(tempoColeta, {
    tempoColeta, tensaoRecente, correnteRecente, posHRecente, posVRecente,
    velocidadeAtual, bateriaAtual, tensaoAtual,
    sensorCor, sensorEsquerda, sensorDireita, sensorFrontal,
  });

  if (buffer.size > MAX_BUFFER_SIZE) {
    const oldestKey = buffer.keys().next().value;
    buffer.delete(oldestKey);
  }
}

function limpar() {
  buffer.clear();
}

function drenar() {
  const rows = Array.from(buffer.values());
  buffer.clear();
  return rows;
}

export default { registrar, limpar, drenar };
