// Representa o estado inicial do labirinto.
export function createVisibleMaze(rows, cols) {
  return Array(rows).fill()
    .map(() => Array(cols).fill(2));
}

// Gera uma chave única para identificar células visitadas.
export function getCellKey(row, col) {
  return `${row}-${col}`;
}

// Verifica se uma posição está dentro dos limites do labirinto.
export function isInsideMaze(row, col, rows, cols) {
  return (
    row >= 0 && row < rows && col >= 0 && col < cols
  );
}

// Verifica se o bloco verde (micromouse) pode se mover para a próxima célula.
export function canMove(current, next, maze, rows, cols) {

  const inside = isInsideMaze(next.row, next.col, rows, cols);
  if (!inside) {
    return false;
  }
  if (maze[next.row][next.col] === 1) {
    return false;
  }
  const rowDiff = Math.abs(next.row - current.row);
  const colDiff = Math.abs(next.col - current.col);

  return rowDiff + colDiff === 1;
}

// Define a cor de cada célula do labirinto.
export function getCellColor(cell, visited) {

  if (visited) {
    return "#90ee90";
  }

  if (cell === 2) {
    return "#707070";
  }

  if (cell === 1) {
    return "black";
  }

  return "white";
}
