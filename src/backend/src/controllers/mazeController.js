/**
 * Mantém o estado em memória do labirinto e expõe:
 *  - handlers para as rotas HTTP de /api/maze/*
 *  - função de atualização chamada pelo esp32Controller após processar telemetria
 */

import { processTelemetry } from '../services/mazeService.js';
import { buildDatabaseRecord } from '../models/mazeModel.js';
import { broadcastMazeState } from '../ws/reactBroadcaster.js';

let latestMazeState = null;

const positionHistory = [];
const MAX_HISTORY = 500;

export function getLatestMazeState() {
  return latestMazeState;
}

export function getPositionHistory() {
  return positionHistory;
}

/**
 * Processa um payload bruto da ESP32, atualiza o estado em memória
 * e distribui o resultado ao frontend.
 * Chamado pelo esp32Controller a cada mensagem recebida.
 *
 * @param {object} rawPayload
 */
export function handleTelemetryUpdate(rawPayload) {
  const mazeState = processTelemetry(rawPayload); // lança se inválido

  // Atualiza estado
  latestMazeState = mazeState;

  if (mazeState.mousePosition) {
    positionHistory.push({ ...mazeState.mousePosition, timestamp: mazeState.timestamp });
    if (positionHistory.length > MAX_HISTORY) positionHistory.shift();
  }

  const dbRecord = buildDatabaseRecord(mazeState);
  // TODO: await db.mazeStates.insert(dbRecord);
  void dbRecord;

  // Distribui ao frontend
  broadcastMazeState(mazeState);
}

/**
 * GET /api/maze/state
 * Retorna o último estado processado do labirinto.
 */
export function getMazeState(req, res, sendJson) {
  if (!latestMazeState) {
    sendJson(res, 204, { message: 'Nenhum estado disponível ainda.' });
    return;
  }
  sendJson(res, 200, latestMazeState);
}

/**
 * GET /api/maze/history
 * Retorna o histórico de posições do rato.
 */
export function getMazeHistory(req, res, sendJson) {
  sendJson(res, 200, { history: positionHistory });
}