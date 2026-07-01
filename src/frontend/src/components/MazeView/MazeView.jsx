import styles from './MazeView.module.css';

// grid: 0=caminho, 1=parede, 2=desconhecido (live via mapa WS; History via reconstrução TELEMETRIA+TRAJETO)
function cellType(row, col, grid, position, goal, start, visitedPath) {
  const cell = grid[row][col];
  if (cell === 1) return 'wall';
  if (position && position[0] === row && position[1] === col) return 'mouse';
  if (cell === 2) return 'unknown';
  if (goal && goal[0] === row && goal[1] === col) return 'goal';
  if (start && start[0] === row && start[1] === col) return 'start';
  const visited = visitedPath.some(([r, c]) => r === row && c === col);
  if (visited) return 'visited';
  return 'path';
}

export function MazeView({ grid, position, goal, start, visitedPath, status }) {
  if (!grid) return null;

  const rows = grid.length;
  const cols = grid[0].length;

  return (
    <div className={styles.wrapper}>
      <div
        className={styles.maze}
        style={{ '--cols': cols, '--rows': rows }}
      >
        {grid.map((row, r) =>
          row.map((_, c) => {
            const type = cellType(r, c, grid, position, goal, start, visitedPath);
            const isGoalSuccess = type === 'goal' && status === 'success';
            return (
              <div
                key={`${r}-${c}`}
                className={`${styles.cell} ${styles[type]} ${isGoalSuccess ? styles.goalSuccess : ''}`}
                title={type === 'mouse' ? `Mouse (${r},${c})` : type === 'goal' ? 'Objetivo' : ''}
              />
            );
          })
        )}
      </div>
      <div className={styles.legend}>
        <span className={`${styles.dot} ${styles.mouseDot}`} /> Mouse
        <span className={`${styles.dot} ${styles.goalDot}`} /> Objetivo
        <span className={`${styles.dot} ${styles.visitedDot}`} /> Caminho
        <span className={`${styles.dot} ${styles.wallDot}`} /> Parede
        <span className={`${styles.dot} ${styles.unknownDot}`} /> Desconhecido
      </div>
    </div>
  );
}
