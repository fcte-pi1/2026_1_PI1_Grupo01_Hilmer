export const CELL = Object.freeze({ PASSAGE: 0, WALL: 1, UNKNOWN: 2 });

export const MAZE_SIZES = Object.freeze({
  '4x4':   4,
  '8x8':   8,
  '16x16': 16,
});

/**
 * Recebe o estado processado e retorna um documento estruturado pronto
 * para ser inserido no banco de dados.
 *
 * Quando o banco for definido, chamar:
 *   await db.mazeStates.insert(buildDatabaseRecord(state));
 *
 * @param {import('../services/mazeService.js').ProcessedMazeState} state
 * @returns {MazeDatabaseRecord}
 */
export function buildDatabaseRecord(state) {
  const { tipoLabirinto, mousePosition, visitedCount, totalCells, cells, metrics, timestamp } = state;
  return {
    timestamp,
    tipoLabirinto,
    mousePosition,
    explorationRate: totalCells > 0 ? visitedCount / totalCells : 0,
    visitedCount,
    totalCells,
    metrics,
    // Apenas células visitadas — reduz volume de escrita no banco
    visitedCells: cells.flat().filter(c => c.visited).map(c => ({
      row:       c.row,
      col:       c.col,
      walls:     c.walls,
      exits:     c.exits,
      exitCount: c.exitCount,
    })),
  };
}