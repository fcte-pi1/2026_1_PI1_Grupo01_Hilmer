/**
 * mouseService.js
 *
 * Serviço de navegação do Micromouse.
 * FloodFill básico (next-move) + análise de tentativa (ida/volta/ótimo).
 */

const DIRECTIONS = [
  { dr: -1, dc: 0, action: 'UP' },
  { dr: 1, dc: 0, action: 'DOWN' },
  { dr: 0, dc: -1, action: 'LEFT' },
  { dr: 0, dc: 1, action: 'RIGHT' },
];

// Leitura baixa (objeto próximo) indica parede na célula vizinha.
const WALL_DISTANCE_MAX_MM = 150;

const DIRECTION_OFFSETS = {
  NORTE: { front: [-1, 0], left: [0, -1], right: [0, 1] },
  SUL: { front: [1, 0], left: [0, 1], right: [0, -1] },
  LESTE: { front: [0, 1], left: [-1, 0], right: [1, 0] },
  OESTE: { front: [0, -1], left: [1, 0], right: [-1, 0] },
};

function getMazeSizeFromTipo(tipoLabirinto) {
  const map = { '4x4': 4, '8x8': 8, '16x16': 16 };
  return map[tipoLabirinto] ?? 16;
}

function normalizeCoords(posH, posV, mazeSize) {
  let col = Number(posH);
  let row = Number(posV);

  if (col >= mazeSize || row >= mazeSize) {
    col -= 1;
    row -= 1;
  }

  col = Math.max(0, Math.min(mazeSize - 1, col));
  row = Math.max(0, Math.min(mazeSize - 1, row));

  return { row, col };
}

function getCenterCells(mazeSize) {
  const mid = Math.floor(mazeSize / 2);
  return [
    { row: mid - 1, col: mid - 1 },
    { row: mid - 1, col: mid },
    { row: mid, col: mid - 1 },
    { row: mid, col: mid },
  ];
}

function getStartCell(path = []) {
  const firstStep = path[0];
  if (!firstStep) {
    return { row: 0, col: 0 };
  }

  return { row: firstStep.row, col: firstStep.col };
}

function isCenterCell(row, col, mazeSize) {
  return getCenterCells(mazeSize).some((cell) => cell.row === row && cell.col === col);
}

function readTrajetoField(step, ...keys) {
  for (const key of keys) {
    if (step?.[key] !== undefined && step?.[key] !== null) return step[key];
  }
  return undefined;
}

function readTelemetriaField(entry, ...keys) {
  for (const key of keys) {
    if (entry?.[key] !== undefined && entry?.[key] !== null) return entry[key];
  }
  return undefined;
}

function matchTelemetriaToTrajeto(trajeto, telemetria) {
  const sortedTrajeto = [...trajeto].sort(
    (a, b) => Number(readTrajetoField(a, 'passo')) - Number(readTrajetoField(b, 'passo')),
  );
  const sortedTelemetria = [...telemetria].sort(
    (a, b) =>
      new Date(readTelemetriaField(a, 'tempocoleta', 'tempoColeta')).getTime() -
      new Date(readTelemetriaField(b, 'tempocoleta', 'tempoColeta')).getTime(),
  );

  return sortedTrajeto.map((step, index) => {
    const posH = Number(readTrajetoField(step, 'pos_h', 'posH'));
    const posV = Number(readTrajetoField(step, 'pos_v', 'posV'));

    const matched =
      sortedTelemetria.find((entry) => {
        const telH = Number(readTelemetriaField(entry, 'poshrecente', 'posHRecente'));
        const telV = Number(readTelemetriaField(entry, 'posvrecente', 'posVRecente'));
        return telH === posH && telV === posV;
      }) ?? sortedTelemetria[index] ?? null;

    return {
      passo: Number(readTrajetoField(step, 'passo')),
      pos_h: posH,
      pos_v: posV,
      direcao: readTrajetoField(step, 'direcao'),
      sensores: matched
        ? {
            frontal: Number(
              readTelemetriaField(matched, 'sensorfrontal', 'sensorFrontal') ?? 999,
            ),
            esquerda: Number(
              readTelemetriaField(matched, 'sensoresquerda', 'sensorEsquerda') ?? 999,
            ),
            direita: Number(
              readTelemetriaField(matched, 'sensordireita', 'sensorDireita') ?? 999,
            ),
          }
        : null,
    };
  });
}

function markWallIfSensor(grid, row, col, dr, dc, distance, mazeSize) {
  if (!Number.isFinite(distance) || distance >= WALL_DISTANCE_MAX_MM) return;
  const nr = row + dr;
  const nc = col + dc;
  if (nr >= 0 && nr < mazeSize && nc >= 0 && nc < mazeSize && grid[nr][nc] !== 0) {
    grid[nr][nc] = 1;
  }
}

function buildCellGrid(steps, mazeSize) {
  const grid = Array.from({ length: mazeSize }, () => Array(mazeSize).fill(2));

  for (let i = 0; i < mazeSize; i += 1) {
    grid[0][i] = 1;
    grid[mazeSize - 1][i] = 1;
    grid[i][0] = 1;
    grid[i][mazeSize - 1] = 1;
  }

  getCenterCells(mazeSize).forEach(({ row, col }) => {
    grid[row][col] = 0;
  });

  steps.forEach((step) => {
    const { row, col } = normalizeCoords(step.pos_h, step.pos_v, mazeSize);
    grid[row][col] = 0;

    if (!step.sensores || !step.direcao) return;

    const offsets = DIRECTION_OFFSETS[step.direcao];
    if (!offsets) return;

    markWallIfSensor(
      grid,
      row,
      col,
      offsets.front[0],
      offsets.front[1],
      step.sensores.frontal,
      mazeSize,
    );
    markWallIfSensor(
      grid,
      row,
      col,
      offsets.left[0],
      offsets.left[1],
      step.sensores.esquerda,
      mazeSize,
    );
    markWallIfSensor(
      grid,
      row,
      col,
      offsets.right[0],
      offsets.right[1],
      step.sensores.direita,
      mazeSize,
    );
  });

  return grid;
}

function trajetoToPath(trajeto, mazeSize) {
  return [...trajeto]
    .sort((a, b) => Number(readTrajetoField(a, 'passo')) - Number(readTrajetoField(b, 'passo')))
    .map((step) => {
      const { row, col } = normalizeCoords(
        readTrajetoField(step, 'pos_h', 'posH'),
        readTrajetoField(step, 'pos_v', 'posV'),
        mazeSize,
      );
      return { row, col };
    });
}

function splitTrajetoPhases(trajeto, mazeSize) {
  const path = trajetoToPath(trajeto, mazeSize);
  if (!path.length) {
    return { outbound: [], returnPath: [] };
  }

  let centerIdx = path.findIndex(({ row, col }) => isCenterCell(row, col, mazeSize));
  if (centerIdx < 0) centerIdx = path.length - 1;

  const start = getStartCell(path);
  let returnEndIdx = path.findIndex(
    ({ row, col }, index) =>
      index > centerIdx && row === start.row && col === start.col,
  );
  if (returnEndIdx < 0) returnEndIdx = path.length - 1;

  const toCoords = (entries) => entries.map(({ row, col }) => [row, col]);

  return {
    outbound: toCoords(path.slice(0, centerIdx + 1)),
    returnPath: toCoords(path.slice(centerIdx, returnEndIdx + 1)),
  };
}

function selectShorterPath(outbound, returnPath) {
  if (!outbound.length) return returnPath;
  if (!returnPath.length) return outbound;
  return returnPath.length < outbound.length ? returnPath : outbound;
}

function analyzeAttempt(trajeto, telemetria, tipoLabirinto) {
  const mazeSize = getMazeSizeFromTipo(tipoLabirinto);
  const matchedSteps = matchTelemetriaToTrajeto(trajeto, telemetria);
  const grid = buildCellGrid(matchedSteps, mazeSize);
  const { outbound, returnPath } = splitTrajetoPhases(trajeto, mazeSize);
  const optimalPath = selectShorterPath(outbound, returnPath);
  const start = getStartCell(trajetoToPath(trajeto, mazeSize));
  const mid = Math.floor(mazeSize / 2);

  return {
    mazeSize,
    grid,
    start: [start.row, start.col],
    goal: [mid - 1, mid - 1],
    outboundPath: outbound,
    returnPath,
    optimalPath,
    outboundSteps: outbound.length,
    returnSteps: returnPath.length,
  };
}

function processFloodFill(currentPosition, mazeMatrix) {
  const rows = mazeMatrix.length;
  const cols = mazeMatrix[0].length;
  const goalRow = rows - 2;
  const goalCol = cols - 2;

  const visited = Array.from({ length: rows }, () => Array(cols).fill(false));
  const parent = Array.from({ length: rows }, () => Array(cols).fill(null));
  const queue = [{ r: currentPosition.row, c: currentPosition.col }];
  visited[currentPosition.row][currentPosition.col] = true;

  let found = false;

  while (queue.length > 0 && !found) {
    const { r, c } = queue.shift();
    if (r === goalRow && c === goalCol) {
      found = true;
      break;
    }

    for (const { dr, dc } of DIRECTIONS) {
      const nr = r + dr;
      const nc = c + dc;
      if (
        nr >= 0 &&
        nr < rows &&
        nc >= 0 &&
        nc < cols &&
        !visited[nr][nc] &&
        mazeMatrix[nr][nc] === 0
      ) {
        visited[nr][nc] = true;
        parent[nr][nc] = { r, c };
        queue.push({ r: nr, c: nc });
      }
    }
  }

  if (!found) {
    return { nextPosition: currentPosition, action: 'STUCK' };
  }

  let cur = { r: goalRow, c: goalCol };
  let firstStep = null;

  while (parent[cur.r][cur.c] !== null) {
    const prev = parent[cur.r][cur.c];
    if (prev.r === currentPosition.row && prev.c === currentPosition.col) {
      firstStep = cur;
      break;
    }
    cur = prev;
  }

  if (!firstStep) {
    return { nextPosition: currentPosition, action: 'ALREADY_AT_GOAL' };
  }

  const dr = firstStep.r - currentPosition.row;
  const dc = firstStep.c - currentPosition.col;
  const dir = DIRECTIONS.find((d) => d.dr === dr && d.dc === dc);

  return {
    nextPosition: { row: firstStep.r, col: firstStep.c },
    action: dir?.action ?? 'MOVE',
  };
}

export default {
  processFloodFill,
  analyzeAttempt,
  buildCellGrid,
  selectShorterPath,
};
