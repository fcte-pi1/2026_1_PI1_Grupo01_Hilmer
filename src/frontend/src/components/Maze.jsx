import { Stage, Layer, Rect, Circle } from "react-konva";
import { useState } from "react";
import { maze } from "../utils/mazeModel";
import { createVisibleMaze, getCellKey, getCellColor } from "../utils/mazeHelpers";
import { parsePath } from "../utils/mazeDataParser";
import { startMazeAnimation } from "../utils/mazeAnimation";

export default function Maze({ rows, cols, cell_size}) {

  /* 
    Da linha 14 até 23 fazem a função de marcar posição inicial
    após a renderização
  */
  const [robot, setRobot] = useState({ row: 1, col: 1 });

  const [visibleMaze, setVisibleMaze] = useState( createVisibleMaze(rows, cols));

  // Armazena células já percorridas.
  const [visitedCells, setVisitedCells] = useState(new Set());

  const [input, setInput] = useState("");

  const goal = { row: 31, col: 25 };

  /*
    Revela as células vizinhas
    ao redor do robô
  */
  function revealAround(row, col) {
    const directions = [
      [0, 0],
      [-1, 0],
      [1, 0],
      [0, -1],
      [0, 1],
    ];
    setVisibleMaze(prev => {
      const updated = prev.map(r => [...r]);
      directions.forEach(([dr, dc]) => {

        const nr = row + dr;
        const nc = col + dc;

        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
          updated[nr][nc] = maze[nr][nc];
        }
      });
      return updated;
    });
  }

  // Marca células visitadas pelo micromouse.
  function markVisited(row, col) {
    setVisitedCells(prev => {
      const updated = new Set(prev);
      updated.add( getCellKey(row, col));
      
      return updated;
    });
  }

  // Inicia animação do trajeto
  function handleStartAnimation() {
    startMazeAnimation({ input, maze, rows, cols,
      goal, parsePath, revealAround, markVisited,
      setRobot,
    });
  }

  return (
    <div className="maze-container">
      <div className="maze-controls">
        <textarea
          rows={10}
          cols={35}
          value={input}
          onChange={(e) =>
            setInput(e.target.value)
          }
        />
        <button onClick={handleStartAnimation}>
          Iniciar
        </button>
      </div>
      <Stage width={cols * cell_size} height={rows * cell_size}>
        <Layer>
          {visibleMaze.map((row, r) =>
            row.map((cell, c) => {
              const visited =
                visitedCells.has(
                  getCellKey(r, c)
                );
              return (
                <Rect
                  key={`${r}-${c}`}
                  x={c * cell_size}
                  y={r * cell_size}
                  width={cell_size}
                  height={cell_size}
                  fill={
                    getCellColor(
                      cell,
                      visited
                    )
                  }
                  stroke="#bdbdbd"
                  strokeWidth={0.5}
                />
              );
            })
          )}
          <Circle
            x={ goal.col * cell_size + cell_size / 2 }
            y={ goal.row * cell_size + cell_size / 2 }
            radius={cell_size / 3}
            fill="red"
          />
          <Circle
            x={ robot.col * cell_size + cell_size / 2 }
            y={ robot.row * cell_size + cell_size / 2 }
            radius={cell_size / 3}
            fill="green"
          />
        </Layer>
      </Stage>
    </div>
  );
}