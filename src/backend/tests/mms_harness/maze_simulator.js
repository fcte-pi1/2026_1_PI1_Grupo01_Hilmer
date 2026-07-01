/**
 * Simulador de labirinto compatível com o modelo de paredes do FloodFill.cpp
 * horiz_walls[r+1][c] bloqueia movimento Norte de (r,c)
 * horiz_walls[r][c]   bloqueia movimento Sul  de (r,c)
 * vert_walls[r][c+1]  bloqueia movimento Leste de (r,c)
 * vert_walls[r][c]    bloqueia movimento Oeste de (r,c)
 */

export class MazeSimulator {
  constructor(size, internalWalls = []) {
    this.size = size;
    this.horiz = Array.from({ length: size + 1 }, () => Array(size).fill(false));
    this.vert = Array.from({ length: size }, () => Array(size + 1).fill(false));

    for (let i = 0; i < size; i++) {
      this.horiz[0][i] = true;
      this.horiz[size][i] = true;
      this.vert[i][0] = true;
      this.vert[i][size] = true;
    }

    for (const wall of internalWalls) {
      this.setWall(wall);
    }
  }

  setWall({ type, r, c }) {
    if (type === "horiz") this.horiz[r][c] = true;
    if (type === "vert") this.vert[r][c] = true;
  }

  /** dir: 0=Norte, 1=Leste, 2=Sul, 3=Oeste (mesmo convenção do FloodFill.cpp) */
  readSensors(r, c, dir) {
    switch (dir) {
      case 0:
        return {
          front: this.horiz[r + 1][c],
          right: this.vert[r][c + 1],
          left: this.vert[r][c],
        };
      case 1:
        return {
          front: this.vert[r][c + 1],
          right: this.horiz[r][c],
          left: this.horiz[r + 1][c],
        };
      case 2:
        return {
          front: this.horiz[r][c],
          right: this.vert[r][c],
          left: this.vert[r][c + 1],
        };
      case 3:
        return {
          front: this.vert[r][c],
          right: this.horiz[r + 1][c],
          left: this.horiz[r][c],
        };
      default:
        throw new Error(`Direção inválida: ${dir}`);
    }
  }

  moveForward(r, c, dir) {
    switch (dir) {
      case 0:
        return [r + 1, c];
      case 1:
        return [r, c + 1];
      case 2:
        return [r - 1, c];
      case 3:
        return [r, c - 1];
      default:
        throw new Error(`Direção inválida: ${dir}`);
    }
  }

  turnRight(dir) {
    return (dir + 1) % 4;
  }

  turnLeft(dir) {
    return (dir + 3) % 4;
  }

  centerCells() {
    const mid = Math.floor(this.size / 2);
    return [
      [mid - 1, mid - 1],
      [mid - 1, mid],
      [mid, mid - 1],
      [mid, mid],
    ];
  }

  isCenter(r, c) {
    return this.centerCells().some(([cr, cc]) => cr === r && cc === c);
  }
}
