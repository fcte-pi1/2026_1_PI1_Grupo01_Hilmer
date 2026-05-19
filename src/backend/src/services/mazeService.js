/**
 * Mapeamento de coordenadas:
 *   Célula lógica (r, c)  →  posição na matriz expandida (2r+1, 2c+1)
 *   Parede norte de (r,c) →  (2r,   2c+1)
 *   Parede sul  de (r,c)  →  (2r+2, 2c+1)
 *   Parede oeste de (r,c) →  (2r+1, 2c)
 *   Parede leste de (r,c) →  (2r+1, 2c+2)
 */

import { CELL, MAZE_SIZES } from '../models/mazeModel.js';

/**
 * Retorna a dimensão lógica a partir do tipo do labirinto.
 *
 * @param {string} tipoLabirinto
 * @returns {number}
 */
export function getMazeDimension(tipoLabirinto) {
  const dim = MAZE_SIZES[tipoLabirinto];
  if (!dim) {
    throw new Error(
      `Tipo de labirinto inválido: "${tipoLabirinto}". Use '4x4', '8x8' ou '16x16'.`
    );
  }
  return dim;
}

/**
 * Valida a estrutura básica do payload recebido do firmware.
 *
 * @param {object} telemetria
 */
export function validateTelemetry(telemetria) {
  if (!telemetria || typeof telemetria !== 'object') {
    throw new Error('Telemetria inválida: payload não é um objeto.');
  }
  if (!telemetria.tipoLabirinto) {
    throw new Error('Telemetria inválida: campo "tipoLabirinto" ausente.');
  }
  if (!Array.isArray(telemetria.mapa) || telemetria.mapa.length === 0) {
    throw new Error('Telemetria inválida: campo "mapa" ausente ou vazio.');
  }
  const dimension = getMazeDimension(telemetria.tipoLabirinto);
  const expected  = 2 * dimension + 1;
  if (telemetria.mapa.length !== expected) {
    throw new Error(
      `Telemetria inválida: mapa tem ${telemetria.mapa.length} linhas, ` +
      `esperado ${expected} para "${telemetria.tipoLabirinto}".`
    );
  }
}

/**
 * Analisa a matriz expandida e extrai, para cada célula lógica (r, c),
 * quais paredes existem e quais saídas estão disponíveis.
 *
 * @param {number[][]} rawMatrix
 * @param {number}     dimension
 * @returns {CellInfo[][]}
 */
export function extractCellWalls(rawMatrix, dimension) {
  const cells = [];

  for (let r = 0; r < dimension; r++) {
    const row = [];
    for (let c = 0; c < dimension; c++) {
      const mr = 2 * r + 1;
      const mc = 2 * c + 1;

      const northWall = rawMatrix[mr - 1]?.[mc]     === CELL.WALL;
      const southWall = rawMatrix[mr + 1]?.[mc]     === CELL.WALL;
      const westWall  = rawMatrix[mr]?.[mc - 1]     === CELL.WALL;
      const eastWall  = rawMatrix[mr]?.[mc + 1]     === CELL.WALL;
      const cellValue = rawMatrix[mr]?.[mc]         ?? CELL.UNKNOWN;

      row.push({
        row: r,
        col: c,
        walls:     { north: northWall, east: eastWall, south: southWall, west: westWall },
        exits:     { north: !northWall, east: !eastWall, south: !southWall, west: !westWall },
        exitCount: [!northWall, !eastWall, !southWall, !westWall].filter(Boolean).length,
        state:     cellValue === CELL.WALL    ? 'wall'
                 : cellValue === CELL.UNKNOWN ? 'unknown'
                 : 'passage',
      });
    }
    cells.push(row);
  }

  return cells;
}

/**
 * Retorna uma matriz booleana indicando quais células lógicas foram visitadas.
 * Uma célula é visitada quando seu valor na matriz expandida é PASSAGE (0).
 *
 * @param {number[][]} rawMatrix
 * @param {number}     dimension
 * @returns {boolean[][]}
 */
export function markVisitedCells(rawMatrix, dimension) {
  const visited = [];
  for (let r = 0; r < dimension; r++) {
    const row = [];
    for (let c = 0; c < dimension; c++) {
      const mr = 2 * r + 1;
      const mc = 2 * c + 1;
      row.push(rawMatrix[mr]?.[mc] === CELL.PASSAGE);
    }
    visited.push(row);
  }
  return visited;
}

/**
 * Extrai a posição atual do rato.
 * Prefere posX/posY explícitos do firmware; caso ausentes, usa heurística
 * de última célula PASSAGE encontrada na varredura.
 *
 * @param {object}     telemetria
 * @param {number[][]} rawMatrix
 * @param {number}     dimension
 * @returns {{ row: number, col: number } | null}
 */
export function extractMousePosition(telemetria, rawMatrix, dimension) {
  if (typeof telemetria.posX === 'number' && typeof telemetria.posY === 'number') {
    return {
      row: Math.min(Math.max(telemetria.posY, 0), dimension - 1),
      col: Math.min(Math.max(telemetria.posX, 0), dimension - 1),
    };
  }

  let lastPassage = null;
  for (let r = 0; r < dimension; r++) {
    for (let c = 0; c < dimension; c++) {
      if (rawMatrix[2 * r + 1]?.[2 * c + 1] === CELL.PASSAGE) {
        lastPassage = { row: r, col: c };
      }
    }
  }
  return lastPassage;
}

/**
 * Processa um payload de telemetria completo e retorna o estado enriquecido
 *
 * @param {object} telemetria - Payload bruto da ESP32 / simulador
 * @returns {ProcessedMazeState}
 */
export function processTelemetry(telemetria) {
  validateTelemetry(telemetria);

  const dimension  = getMazeDimension(telemetria.tipoLabirinto);
  const rawMatrix  = telemetria.mapa;

  const cellWalls  = extractCellWalls(rawMatrix, dimension);
  const visitedMap = markVisitedCells(rawMatrix, dimension);
  const mousePos   = extractMousePosition(telemetria, rawMatrix, dimension);

  const cells      = cellWalls.map((row, r) =>
    row.map((cell, c) => ({ ...cell, visited: visitedMap[r][c] }))
  );

  const totalCells   = dimension * dimension;
  const visitedCount = visitedMap.flat().filter(Boolean).length;

  return {
    timestamp:     new Date().toISOString(),
    tipoLabirinto: telemetria.tipoLabirinto,
    dimension,
    mousePosition: mousePos,
    visitedCount,
    totalCells,
    cells,
    rawMatrix,
    metrics: {
      bateriaConsumo:  Number(telemetria.bateriaConsumo)  || 0,
      velocidadeMedia: Number(telemetria.velocidadeMedia) || 0,
      tempoConclusao:  Number(telemetria.tempoConclusao)  || 0,
      desafioCumprido: telemetria.desafioCumprido         || 'N',
    },
  };
}