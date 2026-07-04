/**
 * Conversão de coordenadas de mapa para célula do labirinto.
 *
 * Formatos suportados (detectados automaticamente em visitedPath):
 * - bordered: mapa mazeSize+2, celulaParaMapa(r,c) => [r+1, c+1]
 * - corridor: mapa expandido com índices ímpares e passos de 2
 */

export const MAP_FORMAT = {
  BORDERED: 'bordered',
  CORRIDOR: 'corridor',
};

function inferDirection(previous, current, next) {
  const origin = previous ?? current;
  const destination = next ?? current;
  const rowDelta = destination[0] - origin[0];
  const colDelta = destination[1] - origin[1];

  if (rowDelta < 0) return 'NORTE';
  if (rowDelta > 0) return 'SUL';
  if (colDelta > 0) return 'LESTE';
  if (colDelta < 0) return 'OESTE';
  return 'NORTE';
}

export function detectVisitedPathFormat(visitedPath, mazeSize) {
  if (!Array.isArray(visitedPath) || visitedPath.length === 0) {
    return MAP_FORMAT.BORDERED;
  }

  const hasEvenCoord = visitedPath.some((point) => {
    const row = Number(point?.[0]);
    const col = Number(point?.[1]);
    return row % 2 === 0 || col % 2 === 0;
  });
  if (hasEvenCoord) {
    return MAP_FORMAT.BORDERED;
  }

  const hasLargeCoord = visitedPath.some((point) => {
    const row = Number(point?.[0]);
    const col = Number(point?.[1]);
    return row > mazeSize + 1 || col > mazeSize + 1;
  });
  if (hasLargeCoord) {
    return MAP_FORMAT.CORRIDOR;
  }

  for (let index = 1; index < visitedPath.length; index += 1) {
    const rowDelta = Math.abs(Number(visitedPath[index][0]) - Number(visitedPath[index - 1][0]));
    const colDelta = Math.abs(Number(visitedPath[index][1]) - Number(visitedPath[index - 1][1]));
    if (rowDelta === 2 || colDelta === 2) {
      return MAP_FORMAT.CORRIDOR;
    }
  }

  return MAP_FORMAT.BORDERED;
}

export function mapPointToCell(row, col, format, mazeSize) {
  let cellRow = Number(row);
  let cellCol = Number(col);

  if (format === MAP_FORMAT.CORRIDOR) {
    cellRow = (cellRow - 1) / 2;
    cellCol = (cellCol - 1) / 2;
  } else if (cellRow >= 1 && cellCol >= 1 && cellRow <= mazeSize + 1 && cellCol <= mazeSize + 1) {
    cellRow -= 1;
    cellCol -= 1;
  }

  return [cellRow, cellCol];
}

/**
 * Converte visitedPath (mapa ou célula) em passos de TRAJETO em célula.
 */
export function visitedPathToCellSteps(visitedPath = [], mazeSize = 16) {
  if (!Array.isArray(visitedPath) || visitedPath.length === 0) {
    return [];
  }

  const format = detectVisitedPathFormat(visitedPath, mazeSize);

  const cells = visitedPath.map((point) => mapPointToCell(
    point[0],
    point[1],
    format,
    mazeSize,
  ));

  return cells.map((position, index) => ({
    passo: index + 1,
    pos_h: position[1],
    pos_v: position[0],
    direcao: inferDirection(
      index > 0 ? cells[index - 1] : null,
      position,
      cells[index + 1] ?? null,
    ),
  }));
}
