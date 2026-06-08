/**
 * mouseService.js
 *
 * Serviço de navegação do Micromouse.
 * Implementa FloodFill básico para cálculo do próximo movimento.
 *
 * TODO: integrar com a implementação C++ (FloodFill.cpp) via WASM ou subprocess
 *       quando o firmware estiver pronto. Por ora, a lógica JS aqui serve
 *       para testes de integração e simulação no frontend.
 */

const DIRECTIONS = [
  { dr: -1, dc: 0, action: 'UP' },
  { dr: 1,  dc: 0, action: 'DOWN' },
  { dr: 0,  dc: -1, action: 'LEFT' },
  { dr: 0,  dc: 1,  action: 'RIGHT' },
];

/**
 * Retorna o próximo passo do caminho calculado por BFS simples.
 * Recebe a posição atual [row, col] e a matriz do labirinto (0=livre, 1=parede).
 *
 * @param {{ row: number, col: number }} currentPosition
 * @param {number[][]} mazeMatrix
 * @returns {{ nextPosition: {row,col}, action: string }}
 */
function processFloodFill(currentPosition, mazeMatrix) {
  const rows = mazeMatrix.length;
  const cols = mazeMatrix[0].length;
  const goalRow = rows - 2;
  const goalCol = cols - 2;

  // BFS do ponto atual até o goal
  const visited = Array.from({ length: rows }, () => Array(cols).fill(false));
  const parent = Array.from({ length: rows }, () => Array(cols).fill(null));
  const queue = [{ r: currentPosition.row, c: currentPosition.col }];
  visited[currentPosition.row][currentPosition.col] = true;

  let found = false;

  while (queue.length > 0 && !found) {
    const { r, c } = queue.shift();
    if (r === goalRow && c === goalCol) { found = true; break; }

    for (const { dr, dc } of DIRECTIONS) {
      const nr = r + dr;
      const nc = c + dc;
      if (nr >= 0 && nr < rows && nc >= 0 && nc < cols
          && !visited[nr][nc] && mazeMatrix[nr][nc] === 0) {
        visited[nr][nc] = true;
        parent[nr][nc] = { r, c };
        queue.push({ r: nr, c: nc });
      }
    }
  }

  if (!found) {
    // TODO: tratar labirinto sem solução — por ora retorna posição atual
    return { nextPosition: currentPosition, action: 'STUCK' };
  }

  // Reconstrói caminho e retorna o primeiro passo
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
  const dir = DIRECTIONS.find(d => d.dr === dr && d.dc === dc);

  return {
    nextPosition: { row: firstStep.r, col: firstStep.c },
    action: dir?.action ?? 'MOVE',
  };
}

export default { processFloodFill };