/**
 * attemptService.js
 *
 * Orquestra persistência de tentativa a partir da telemetria ao vivo ou HTTP.
 * Idempotência por numTentativa efêmero da ESP. TRAJETO gravado em célula
 * (0..N-1) via liveTrajetoBuffer ou conversão de visitedPath.
 */

import simulationService from './simulationService.js';
import liveTelemetryBuffer from './liveTelemetryBuffer.js';
import liveTrajetoBuffer from './liveTrajetoBuffer.js';
import { visitedPathToCellSteps } from '../utils/mazeCoordinates.js';
import { clampBattery, toFiniteNumber } from '../utils/validation.js';

/** @type {Map<number|string, object>} */
const persistedByEspAttempt = new Map();

/** @type {Map<number|string, Promise<{ alreadyPersisted: boolean, record: object }>>} */
const inflightByEspAttempt = new Map();

function shouldResetRun(status) {
  return status === 'waiting' || status === 'waiting_start';
}

function resolveTipoLabirinto(payload) {
  const tipo = payload?.tipoLabirinto ?? payload?.historico?.tipoLabirinto;
  if (tipo === '4x4' || tipo === '8x8' || tipo === '16x16') {
    return tipo;
  }

  const mazeSize = Number(payload?.mazeSize ?? payload?.historico?.mazeSize);
  if (mazeSize === 4) return '4x4';
  if (mazeSize === 8) return '8x8';
  return '16x16';
}

function resolveMazeSize(payload) {
  const mazeSize = Number(payload?.mazeSize ?? payload?.historico?.mazeSize);
  if (mazeSize === 4 || mazeSize === 8 || mazeSize === 16) {
    return mazeSize;
  }

  const tipo = resolveTipoLabirinto(payload);
  if (tipo === '4x4') return 4;
  if (tipo === '8x8') return 8;
  return 16;
}

function buildTrajetoSteps(payload) {
  const bufferedSteps = liveTrajetoBuffer.listar();
  if (bufferedSteps.length > 0) {
    return bufferedSteps;
  }

  return visitedPathToCellSteps(payload?.visitedPath ?? [], resolveMazeSize(payload));
}

function buildHistoricoPayload(payload) {
  const historico = payload?.historico ?? {};

  return {
    percentualBateria: clampBattery(
      historico.percentualBateria ?? payload.batteryPercent ?? payload.bateriaAtual,
    ),
    velocidadeMedia: toFiniteNumber(
      historico.velocidadeMedia ?? payload.speedMps ?? payload.velocidadeAtual,
      0,
    ),
    tempoConclusao:
      historico.tempoConclusao
      || payload.tempoConclusao
      || new Date().toISOString(),
    desafioCumprido:
      historico.desafioCumprido === 'NAO' || payload.desafioCumprido === 'NAO'
        ? 'NAO'
        : 'SIM',
    correnteEletrica: toFiniteNumber(
      historico.correnteEletrica ?? payload.correnteEletrica ?? payload.correnteRecente,
      0,
    ),
    tensaoEletrica: toFiniteNumber(
      historico.tensaoEletrica ?? payload.tensaoEletrica ?? payload.tensaoAtual ?? payload.tensaoRecente,
      0,
    ),
    tipoLabirinto: resolveTipoLabirinto(payload),
  };
}

function resolveEspAttemptKey(payload, explicitKey) {
  if (explicitKey !== undefined && explicitKey !== null && explicitKey !== '') {
    return explicitKey;
  }

  if (payload?.numTentativa !== undefined && payload?.numTentativa !== null) {
    return payload.numTentativa;
  }

  return null;
}

function clearRunBuffers() {
  liveTelemetryBuffer.limpar();
  liveTrajetoBuffer.limpar();
}

async function handleLiveTelemetry(payload) {
  if (!payload || typeof payload !== 'object') {
    return null;
  }

  const status = payload.status;

  if (shouldResetRun(status)) {
    clearRunBuffers();
    return null;
  }

  if (payload.tempoColeta) {
    liveTelemetryBuffer.registrar(payload);
  }

  liveTrajetoBuffer.registrar(payload.trajetoAtual);

  if (status === 'success') {
    return persistSuccessfulAttempt(payload);
  }

  return null;
}

async function persistSuccessfulAttempt(payload = {}, options = {}) {
  const espKey = resolveEspAttemptKey(payload, options.espNumTentativa);

  if (espKey !== null && persistedByEspAttempt.has(espKey)) {
    return {
      alreadyPersisted: true,
      record: persistedByEspAttempt.get(espKey),
    };
  }

  if (espKey !== null && inflightByEspAttempt.has(espKey)) {
    return inflightByEspAttempt.get(espKey);
  }

  const run = (async () => {
    const historicoPayload = {
      ...buildHistoricoPayload(payload),
      ...(options.historicoOverrides ?? {}),
    };

    if (!historicoPayload.tempoConclusao || Number.isNaN(Date.parse(historicoPayload.tempoConclusao))) {
      historicoPayload.tempoConclusao = new Date().toISOString();
    }

    const steps = options.trajetoSteps ?? buildTrajetoSteps(payload);
    const record = await simulationService.criarHistorico(historicoPayload);
    const numTentativa = record.numtentativa;

    await simulationService.inserirTelemetriaEmLote(
      liveTelemetryBuffer.drenar(),
      numTentativa,
    );
    await simulationService.inserirTrajetoEmLote(steps, numTentativa);

    if (espKey !== null) {
      persistedByEspAttempt.set(espKey, record);
    }

    liveTrajetoBuffer.limpar();

    return {
      alreadyPersisted: false,
      record,
    };
  })();

  if (espKey !== null) {
    inflightByEspAttempt.set(espKey, run);
  }

  try {
    return await run;
  } finally {
    if (espKey !== null) {
      inflightByEspAttempt.delete(espKey);
    }
  }
}

async function persistFromHttpRequest(body) {
  const espKey = body.espNumTentativa ?? body.numTentativaEsp ?? null;

  if (espKey !== null && espKey !== undefined && persistedByEspAttempt.has(espKey)) {
    return {
      alreadyPersisted: true,
      record: persistedByEspAttempt.get(espKey),
    };
  }

  const mazeSize = resolveMazeSize(body);
  const trajetoSteps = Array.isArray(body.trajeto)
    ? body.trajeto
    : visitedPathToCellSteps(body.visitedPath ?? [], mazeSize);

  return persistSuccessfulAttempt(
    {
      numTentativa: espKey,
      batteryPercent: body.percentualBateria,
      speedMps: body.velocidadeMedia,
      correnteEletrica: body.correnteEletrica,
      tensaoEletrica: body.tensaoEletrica,
      tipoLabirinto: body.tipoLabirinto,
      mazeSize,
      desafioCumprido: body.desafioCumprido,
      tempoConclusao: body.tempoConclusao,
      visitedPath: body.visitedPath,
      historico: body,
    },
    {
      espNumTentativa: espKey,
      historicoOverrides: {
        percentualBateria: clampBattery(body.percentualBateria),
        velocidadeMedia: toFiniteNumber(body.velocidadeMedia, 0),
        tempoConclusao: body.tempoConclusao,
        desafioCumprido: body.desafioCumprido,
        correnteEletrica: toFiniteNumber(body.correnteEletrica, 0),
        tensaoEletrica: toFiniteNumber(body.tensaoEletrica, 0),
        tipoLabirinto: body.tipoLabirinto,
      },
      trajetoSteps: trajetoSteps.length > 0 ? trajetoSteps : undefined,
    },
  );
}

function resetAttemptState() {
  persistedByEspAttempt.clear();
  inflightByEspAttempt.clear();
  clearRunBuffers();
}

export default {
  handleLiveTelemetry,
  persistSuccessfulAttempt,
  persistFromHttpRequest,
  resetAttemptState,
};
