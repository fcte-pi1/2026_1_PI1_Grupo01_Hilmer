/**
 * simulationService.js
 *
 * Camada de serviço: acesso ao banco usando as tabelas reais do schema.
 * Tabelas: HISTORICO, TELEMETRIA, TRAJETO
 */

import pool from '../database/connection.js';

// ─── HISTORICO ────────────────────────────────────────────────────────────────

/**
 * Cria um registro de tentativa no HISTORICO.
 * @param {{
 *   numTentativa: number,
 *   percentualBateria: number,
 *   velocidadeMedia: number,
 *   tempoConclusao: string,   // ISO timestamp
 *   desafioCumprido: 'SIM'|'NAO',
 *   correnteEletrica: number,
 *   tensaoEletrica: number,
 *   tipoLabirinto: '4x4'|'8x8'|'16x16'
 * }} dados
 */
async function criarHistorico(dados) {
  const {
    numTentativa, percentualBateria, velocidadeMedia,
    tempoConclusao, desafioCumprido, correnteEletrica,
    tensaoEletrica, tipoLabirinto,
  } = dados;

  const result = await pool.query(
    `INSERT INTO HISTORICO
       (numTentativa, percentualBateria, velocidadeMedia, tempoConclusao,
        desafioCumprido, correnteEletrica, tensaoEletrica, tipoLabirinto)
     VALUES ($1, $2, $3, $4, $5, $6, $7, $8)
     RETURNING *`,
    [numTentativa, percentualBateria, velocidadeMedia, tempoConclusao,
     desafioCumprido, correnteEletrica, tensaoEletrica, tipoLabirinto]
  );

  return result.rows[0];
}

/**
 * Retorna todos os registros de HISTORICO ordenados pela tentativa mais recente.
 */
async function listarHistorico() {
  const result = await pool.query(
    `SELECT * FROM HISTORICO ORDER BY numTentativa DESC`
  );
  return result.rows;
}

/**
 * Retorna um HISTORICO pelo numTentativa.
 * @param {number} numTentativa
 */
async function buscarHistoricoPorTentativa(numTentativa) {
  const result = await pool.query(
    `SELECT * FROM HISTORICO WHERE numTentativa = $1`,
    [numTentativa]
  );
  return result.rows[0] ?? null;
}

// ─── TELEMETRIA ───────────────────────────────────────────────────────────────

/**
 * Insere um snapshot de telemetria recebido da ESP32.
 * @param {{
 *   numTentativa: number,
 *   tempoColeta: string,       // ISO timestamp
 *   tensaoRecente: number,
 *   correnteRecente: number,
 *   posHRecente: number,
 *   posVRecente: number,
 *   velocidadeAtual: number,
 *   bateriaAtual: number,
 *   tensaoAtual: number,
 *   sensorCor?: string,        // '#RRGGBB' ou null
 *   sensorEsquerda?: number,
 *   sensorDireita?: number,
 *   sensorFrontal?: number,
 * }} dados
 */
async function inserirTelemetria(dados) {
  const {
    numTentativa, tempoColeta, tensaoRecente, correnteRecente,
    posHRecente, posVRecente, velocidadeAtual, bateriaAtual, tensaoAtual,
    sensorCor = null, sensorEsquerda = null, sensorDireita = null, sensorFrontal = null,
  } = dados;

  const result = await pool.query(
    `INSERT INTO TELEMETRIA
       (numTentativa, tempoColeta, tensaoRecente, correnteRecente,
        posHRecente, posVRecente, velocidadeAtual, bateriaAtual, tensaoAtual,
        sensorCor, sensorEsquerda, sensorDireita, sensorFrontal)
     VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12,$13)
     RETURNING *`,
    [numTentativa, tempoColeta, tensaoRecente, correnteRecente,
     posHRecente, posVRecente, velocidadeAtual, bateriaAtual, tensaoAtual,
     sensorCor, sensorEsquerda, sensorDireita, sensorFrontal]
  );

  return result.rows[0];
}

/**
 * Retorna toda a telemetria de uma tentativa, ordenada por tempoColeta.
 * @param {number} numTentativa
 */
async function listarTelemetriaPorTentativa(numTentativa) {
  const result = await pool.query(
    `SELECT * FROM TELEMETRIA
     WHERE numTentativa = $1
     ORDER BY tempoColeta ASC`,
    [numTentativa]
  );
  return result.rows;
}

// ─── TRAJETO ──────────────────────────────────────────────────────────────────

/**
 * Insere um passo do trajeto percorrido pelo mouse.
 * @param {{
 *   numTentativa: number,
 *   passo: number,
 *   pos_h: number,
 *   pos_v: number,
 *   direcao: 'NORTE'|'SUL'|'LESTE'|'OESTE'
 * }} dados
 */
async function inserirPassoTrajeto(dados) {
  const { numTentativa, passo, pos_h, pos_v, direcao } = dados;

  const result = await pool.query(
    `INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
     VALUES ($1, $2, $3, $4, $5)
     RETURNING *`,
    [numTentativa, passo, pos_h, pos_v, direcao]
  );

  return result.rows[0];
}

/**
 * Retorna o trajeto completo de uma tentativa, ordenado pelo passo.
 * @param {number} numTentativa
 */
async function listarTrajetoPorTentativa(numTentativa) {
  const result = await pool.query(
    `SELECT * FROM TRAJETO
     WHERE numTentativa = $1
     ORDER BY passo ASC`,
    [numTentativa]
  );
  return result.rows;
}

export default {
  criarHistorico,
  listarHistorico,
  buscarHistoricoPorTentativa,
  inserirTelemetria,
  listarTelemetriaPorTentativa,
  inserirPassoTrajeto,
  listarTrajetoPorTentativa,
};