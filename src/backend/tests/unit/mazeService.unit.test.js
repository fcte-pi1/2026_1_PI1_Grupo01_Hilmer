/**
 * Execute:
 *   node --test src/backend/tests/unit/mazeService.unit.test.js
 */

import { describe, it } from 'node:test';
import assert from 'node:assert/strict';

import {
  getMazeDimension,
  validateTelemetry,
  extractCellWalls,
  markVisitedCells,
  extractMousePosition,
  processTelemetry,
} from '../../src/services/mazeService.js';

import { CELL } from '../../src/models/mazeModel.js';
import { buildDatabaseRecord } from '../../src/models/mazeModel.js';

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function makeMatrix(dimension) {
  const size = 2 * dimension + 1;
  return Array.from({ length: size }, (_, i) =>
    Array.from({ length: size }, (_, j) =>
      i === 0 || i === size - 1 || j === 0 || j === size - 1
        ? CELL.WALL : CELL.UNKNOWN
    )
  );
}

function makeTelemetry(tipo = '4x4', overrides = {}) {
  const mapa = makeMatrix(getMazeDimension(tipo));
  return {
    tipoLabirinto: tipo,
    bateriaConsumo: '50.0', velocidadeMedia: '0.4',
    tempoConclusao: '10.0', desafioCumprido: 'N',
    mapa, ...overrides,
  };
}

// ---------------------------------------------------------------------------
// getMazeDimension
// ---------------------------------------------------------------------------

describe('getMazeDimension', () => {
  it('retorna 4 para "4x4"',    () => assert.equal(getMazeDimension('4x4'),   4));
  it('retorna 8 para "8x8"',    () => assert.equal(getMazeDimension('8x8'),   8));
  it('retorna 16 para "16x16"', () => assert.equal(getMazeDimension('16x16'), 16));
  it('lança para tipo inválido',() =>
    assert.throws(() => getMazeDimension('5x5'), /inválido/));
});

// ---------------------------------------------------------------------------
// validateTelemetry
// ---------------------------------------------------------------------------

describe('validateTelemetry', () => {
  it('não lança para payload válido 4x4',  () =>
    assert.doesNotThrow(() => validateTelemetry(makeTelemetry('4x4'))));
  it('não lança para payload válido 16x16',() =>
    assert.doesNotThrow(() => validateTelemetry(makeTelemetry('16x16'))));
  it('lança se payload é null',            () =>
    assert.throws(() => validateTelemetry(null), /inválida/));
  it('lança se tipoLabirinto ausente',     () =>
    assert.throws(() => validateTelemetry({ mapa: [[]] }), /tipoLabirinto/));
  it('lança se mapa ausente',              () =>
    assert.throws(() => validateTelemetry({ tipoLabirinto: '4x4' }), /mapa/));
  it('lança se tamanho do mapa não bate',  () => {
    const t = makeTelemetry('4x4');
    t.mapa = t.mapa.slice(0, 3);
    assert.throws(() => validateTelemetry(t), /linhas/);
  });
});

// ---------------------------------------------------------------------------
// extractCellWalls — CA-08.1 + CA-08.4
// ---------------------------------------------------------------------------

describe('extractCellWalls', () => {
  it('retorna matriz dimension×dimension', () => {
    const cells = extractCellWalls(makeMatrix(4), 4);
    assert.equal(cells.length, 4);
    cells.forEach(row => assert.equal(row.length, 4));
  });

  it('CA-08.1 — célula (0,0) tem parede norte e oeste (bordas)', () => {
    const cells = extractCellWalls(makeMatrix(4), 4);
    assert.equal(cells[0][0].walls.north, true);
    assert.equal(cells[0][0].walls.west,  true);
  });

  it('CA-08.4 — exits é o inverso de walls', () => {
    const cells = extractCellWalls(makeMatrix(4), 4);
    const c = cells[0][0];
    assert.equal(c.exits.north, !c.walls.north);
    assert.equal(c.exits.east,  !c.walls.east);
    assert.equal(c.exits.south, !c.walls.south);
    assert.equal(c.exits.west,  !c.walls.west);
  });

  it('CA-08.4 — exitCount correto com 4 saídas abertas', () => {
    const m = makeMatrix(4);
    // célula (1,1): mr=3, mc=3; norte=(2,3), sul=(4,3), oeste=(3,2), leste=(3,4)
    m[2][3] = CELL.UNKNOWN;
    m[4][3] = CELL.UNKNOWN;
    m[3][2] = CELL.UNKNOWN;
    m[3][4] = CELL.UNKNOWN;
    assert.equal(extractCellWalls(m, 4)[1][1].exitCount, 4);
  });

  it('CA-08.4 — exitCount=0 quando 4 paredes fechadas', () => {
    const m = makeMatrix(4);
    m[2][3] = CELL.WALL;
    m[4][3] = CELL.WALL;
    m[3][2] = CELL.WALL;
    m[3][4] = CELL.WALL;
    assert.equal(extractCellWalls(m, 4)[1][1].exitCount, 0);
  });
});

// ---------------------------------------------------------------------------
// markVisitedCells — CA-08.2
// ---------------------------------------------------------------------------

describe('markVisitedCells', () => {
  it('células UNKNOWN não são visitadas', () => {
    const v = markVisitedCells(makeMatrix(4), 4);
    v.forEach(row => row.forEach(val => assert.equal(val, false)));
  });

  it('CA-08.2 — célula PASSAGE é marcada como visitada', () => {
    const m = makeMatrix(4);
    m[3][3] = CELL.PASSAGE; // lógica (1,1)
    assert.equal(markVisitedCells(m, 4)[1][1], true);
  });

  it('célula WALL não é marcada como visitada', () => {
    const m = makeMatrix(4);
    m[3][3] = CELL.WALL;
    assert.equal(markVisitedCells(m, 4)[1][1], false);
  });
});

// ---------------------------------------------------------------------------
// extractMousePosition — CA-08.3
// ---------------------------------------------------------------------------

describe('extractMousePosition', () => {
  it('CA-08.3 — usa posX/posY explícitos', () => {
    const pos = extractMousePosition({ posX: 2, posY: 3 }, makeMatrix(4), 4);
    assert.deepEqual(pos, { row: 3, col: 2 });
  });

  it('CA-08.3 — clipa posição fora dos limites', () => {
    const pos = extractMousePosition({ posX: 99, posY: -5 }, makeMatrix(4), 4);
    assert.equal(pos.col, 3);
    assert.equal(pos.row, 0);
  });

  it('CA-08.3 — fallback para última célula PASSAGE', () => {
    const m = makeMatrix(4);
    m[3][3] = CELL.PASSAGE; // (1,1)
    m[5][5] = CELL.PASSAGE; // (2,2)
    assert.deepEqual(extractMousePosition({}, m, 4), { row: 2, col: 2 });
  });

  it('retorna null se sem PASSAGE e sem posX/posY', () => {
    assert.equal(extractMousePosition({}, makeMatrix(4), 4), null);
  });
});

// ---------------------------------------------------------------------------
// buildDatabaseRecord — CA-08.5
// ---------------------------------------------------------------------------

describe('buildDatabaseRecord', () => {
  it('CA-08.5 — inclui todos os campos obrigatórios', () => {
    const rec = buildDatabaseRecord(processTelemetry(makeTelemetry('4x4')));
    assert.ok(rec.timestamp);
    assert.ok(rec.tipoLabirinto);
    assert.ok(Array.isArray(rec.visitedCells));
    assert.equal(typeof rec.explorationRate, 'number');
    assert.equal(typeof rec.visitedCount,    'number');
    assert.equal(typeof rec.totalCells,      'number');
  });

  it('CA-08.5 — explorationRate entre 0 e 1', () => {
    const rec = buildDatabaseRecord(processTelemetry(makeTelemetry('4x4')));
    assert.ok(rec.explorationRate >= 0 && rec.explorationRate <= 1);
  });

  it('CA-08.5 — visitedCells contém walls, exits e exitCount', () => {
    const t = makeTelemetry('4x4');
    t.mapa[3][3] = CELL.PASSAGE;
    buildDatabaseRecord(processTelemetry(t)).visitedCells.forEach(c => {
      assert.ok('walls'     in c);
      assert.ok('exits'     in c);
      assert.ok('exitCount' in c);
    });
  });
});

// ---------------------------------------------------------------------------
// processTelemetry — pipeline
// ---------------------------------------------------------------------------

describe('processTelemetry', () => {
  it('retorna todos os campos esperados', () => {
    const s = processTelemetry(makeTelemetry('8x8'));
    assert.ok(s.timestamp);
    assert.equal(s.tipoLabirinto, '8x8');
    assert.equal(s.dimension, 8);
    assert.ok(Array.isArray(s.cells));
    assert.ok(Array.isArray(s.rawMatrix));
    assert.equal(typeof s.visitedCount, 'number');
    assert.equal(typeof s.totalCells,   'number');
    assert.ok(s.metrics);
  });

  it('cells tem dimensão correta para 16x16', () => {
    const s = processTelemetry(makeTelemetry('16x16'));
    assert.equal(s.cells.length,    16);
    assert.equal(s.cells[0].length, 16);
  });

  it('totalCells = dimension²', () => {
    assert.equal(processTelemetry(makeTelemetry('8x8')).totalCells, 64);
  });

  it('lança para tipo inválido', () => {
    const t = makeTelemetry('4x4');
    t.tipoLabirinto = '32x32';
    assert.throws(() => processTelemetry(t));
  });

  it('metrics são convertidos para número', () => {
    const { metrics } = processTelemetry(makeTelemetry('4x4'));
    assert.equal(typeof metrics.bateriaConsumo,  'number');
    assert.equal(typeof metrics.velocidadeMedia, 'number');
    assert.equal(typeof metrics.tempoConclusao,  'number');
  });
});