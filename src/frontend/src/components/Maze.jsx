/*

/* Ideias: tratar
as celulas como containers
para o circulo verde se 
mover. --> Move Shape 
to Another Container
  

moveTo() é uma função
que desloca o polígono

para teste, é legal fazer
uma caixa de texto para
inserir a sequência de 
celulas que foram 
inseridas

(0, 0) -> (0, 1) -> (0, 2) -> 
(0, 3) -> (-1, 3)
se tiver parede, não passa
se a cor for vermelha, volte

*/

import { Stage, Layer, Rect, Circle } from "react-konva";
import { useState } from "react";

// serão alterados
const CELL_SIZE = 20;
const ROWS = 15;
const COLS = 15;

/*
  0 -> caminho
  1 -> parede
  2 -> desconhecido
*/

/* maze pra teste */

const maze = [
  [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
  [1,0,0,0,0,0,1,0,0,0,0,0,0,0,1],
  [1,0,1,1,1,0,1,0,1,1,1,1,1,0,1],
  [1,0,1,0,0,0,1,0,0,0,0,0,1,0,1],
  [1,0,1,0,1,1,1,1,1,1,1,0,1,0,1],
  [1,0,1,0,0,0,0,0,0,0,1,0,1,0,1],
  [1,0,1,1,1,1,1,1,1,0,1,0,1,0,1],
  [1,0,0,0,0,0,0,0,1,0,1,0,0,0,1],
  [1,1,1,1,1,1,1,0,1,0,1,1,1,0,1],
  [1,0,0,0,0,0,1,0,1,0,0,0,1,0,1],
  [1,0,1,1,1,0,1,0,1,1,1,0,1,0,1],
  [1,0,1,0,0,0,0,0,0,0,1,0,1,0,1],
  [1,0,1,1,1,1,1,1,1,0,1,0,1,0,1],
  [1,0,0,0,0,0,0,0,1,0,0,0,0,0,1],
  [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1],
];

const createVisibleMaze = () => {

  return Array(ROWS)
    .fill()
    .map(() => Array(COLS).fill(2));

};

export default function Maze() {

  const [robot, setRobot] = useState({row: 1, col: 1});

  const [visibleMaze, setVisibleMaze] =
    useState(createVisibleMaze());

  const [visitedCells, setVisitedCells] =
    useState(new Set());

  const [input, setInput] = useState(`(1,1), (1,2), (1,3), (1,4), (1,5), 
    (1,6), (1,7)`
  );

  const goal = {row: 13, col: 13};

  let key = (row, col) => {
    return `${row}-${col}`;
  }

  /* parser do input */

  let parsePath = (text) => {

    const matches =
      text.match(/\((\d+),\s*(\d+)\)/g);

    if (!matches) return [];

    return matches.map(item => {
      const numbers = item.match(/\d+/g);
      return {
        row: Number(numbers[0]),
        col: Number(numbers[1]),
      };
    });
  }

  /* mostrar os vizinhos */

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
        if (nr >= 0 && nr < ROWS && nc >= 0 && nc < COLS) {  
          updated[nr][nc] = maze[nr][nc];
        }
      });

      return updated;
    });
  }

  /* marcar o caminho*/

  let markVisited = (row, col) => {
    setVisitedCells(prev => {
      const updated = new Set(prev);
      updated.add(key(row, col));
      return updated;
    });
  }

  /* Validar movimento do micromouse */

  let canMove = (current,next) => {

    if (maze[next.row][next.col] === 1) {
      return false;
    }

    const rowDiff = Math.abs(next.row - current.row);
    const colDiff = Math.abs(next.col - current.col);

    return rowDiff + colDiff === 1;
  }

  /* aqui eu adiciono a animação */

  let startAnimation = () => {

    const parsed = parsePath(input);
    if (parsed.length === 0) return;
    let index = 0;
    let currentPosition = parsed[0];

    /*
      posição inicial
    */

    setRobot(currentPosition);
    revealAround(currentPosition.row, currentPosition.col);

    markVisited(
      currentPosition.row,
      currentPosition.col
    );

    index = 1;

    const interval = setInterval(() => {

      if (index >= parsed.length) {
        clearInterval(interval);
        return;
      }
      const next = parsed[index];

      /*
        bateu em parede
      */

      if (maze[next.row][next.col] === 1) {
        revealAround(currentPosition.row, currentPosition.col);
        clearInterval(interval);
        return;
      }
      if (!canMove(currentPosition,next)) {
        clearInterval(interval);
        return;
      }

      // faltar representar o objetivo       
      setRobot(next);
      revealAround(next.row, next.col);
      markVisited(next.row,next.col);

      /*
        chegou no objetivo
      */
      if (next.row === goal.row && next.col === goal.col) {
        clearInterval(interval);
        return;
      }
      currentPosition = next;
      index++;
    }, 300);
  }

}