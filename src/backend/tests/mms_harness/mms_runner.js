import { spawn } from "node:child_process";
import { createInterface } from "node:readline";
import { MazeSimulator } from "./maze_simulator.js";

const DIR_NAMES = ["N", "E", "S", "W"];

export class MmsRunner {
  constructor(executable, scenario) {
    this.executable = executable;
    this.scenario = scenario;
    this.maze = new MazeSimulator(scenario.size, scenario.walls ?? []);
    this.r = 0;
    this.c = 0;
    this.dir = 0;
    this.phase = 0;
    this.phasesReached = [];
    this.movements = [];
    this.setTextUpdates = [];
    this.stderrLogs = [];
    this.steps = 0;
    this.distances = {};
    this.victory = false;
    this.error = null;
  }

  async run(timeoutMs = 120_000) {
    return new Promise((resolve) => {
      const child = spawn(this.executable, [], {
        stdio: ["pipe", "pipe", "pipe"],
        windowsHide: true,
      });

      this._child = child;
      this._finish = null;
      this._finished = false;

      child.stdin.on("error", () => {
        /* EPIPE quando o processo encerra antes da resposta */
      });

      const rl = createInterface({ input: child.stdout });

      const finish = (result) => {
        if (this._finished) return;
        this._finished = true;
        clearTimeout(timer);
        try {
          rl.close();
        } catch {
          /* ignore */
        }
        try {
          child.kill();
        } catch {
          /* processo já encerrado */
        }
        resolve(result);
      };

      this._finish = finish;

      const timer = setTimeout(() => {
        this.error = `Timeout após ${timeoutMs}ms (${this.steps} passos)`;
        finish(this.buildResult("TIMEOUT"));
      }, timeoutMs);

      child.stderr.on("data", (chunk) => {
        const text = chunk.toString();
        for (const line of text.split(/\r?\n/)) {
          if (!line.trim()) continue;
          this.stderrLogs.push(line.trim());
          if (line.includes("Fase 0: Centro alcançado")) {
            this.phase = 1;
            this.phasesReached.push("phase_0");
            this.maybeStopEarly();
          } else if (line.includes("Fase 1: Retorno concluído")) {
            this.phase = 2;
            this.phasesReached.push("phase_1");
            this.maybeStopEarly();
          } else if (line.includes("Fase 2: VITÓRIA")) {
            this.victory = true;
            this.phasesReached.push("phase_2");
            this.maybeStopEarly();
          }
        }
      });

      child.on("close", (code) => {
        if (this.victory) {
          finish(this.buildResult("PASS"));
        } else if (!this.error) {
          this.error = `Processo encerrou com código ${code} sem vitória`;
          finish(this.buildResult("FAIL"));
        }
      });

      child.on("error", (err) => {
        this.error = err.message;
        finish(this.buildResult("FAIL"));
      });

      rl.on("line", (line) => {
        if (this._finished) return;
        const trimmed = line.trim();
        if (!trimmed) return;

        try {
          const response = this.handleCommand(trimmed);
          if (response !== null && !this._finished && child.stdin.writable) {
            child.stdin.write(`${response}\n`);
          }
        } catch (err) {
          this.error = err.message;
          finish(this.buildResult("FAIL"));
        }
      });
    });
  }

  handleCommand(command) {
    if (command === "mazeWidth") {
      return String(this.scenario.size);
    }

    if (command.startsWith("setText ")) {
      const parts = command.split(" ");
      const x = Number(parts[1]);
      const y = Number(parts[2]);
      const value = parts[3];
      this.distances[`${y},${x}`] = Number(value);
      this.setTextUpdates.push({ x, y, value: Number(value) });
      this.maybeStopEarly();
      return null;
    }

    if (command === "wallFront" || command === "wallRight" || command === "wallLeft") {
      const sensors = this.maze.readSensors(this.r, this.c, this.dir);
      const map = { wallFront: sensors.front, wallRight: sensors.right, wallLeft: sensors.left };
      return map[command] ? "true" : "false";
    }

    if (command === "turnRight") {
      this.dir = this.maze.turnRight(this.dir);
      this.movements.push({ type: "turnRight", r: this.r, c: this.c, dir: this.dir });
      return "ok";
    }

    if (command === "turnLeft") {
      this.dir = this.maze.turnLeft(this.dir);
      this.movements.push({ type: "turnLeft", r: this.r, c: this.c, dir: this.dir });
      return "ok";
    }

    if (command === "moveForward") {
      const [nr, nc] = this.maze.moveForward(this.r, this.c, this.dir);
      this.r = nr;
      this.c = nc;
      this.steps += 1;
      this.movements.push({
        type: "moveForward",
        r: this.r,
        c: this.c,
        dir: this.dir,
        dirName: DIR_NAMES[this.dir],
      });
      return "ok";
    }

    return "ok";
  }

  maybeStopEarly() {
    if (!this._finish) return;

    const { stopAfterInitialDraw, stopAfterPhase } = this.scenario;

    if (stopAfterInitialDraw) {
      const expectedCells = this.scenario.size * this.scenario.size;
      if (this.setTextUpdates.length >= expectedCells) {
        const validations = this.runValidations("PASS");
        const allPass = validations.every((v) => v.pass);
        this._finish(this.buildResult(allPass ? "PASS" : "FAIL"));
      }
      return;
    }

    if (stopAfterPhase && this.phasesReached.includes(stopAfterPhase)) {
      const validations = this.runValidations("PASS");
      const allPass = validations.every((v) => v.pass);
      this._finish(this.buildResult(allPass ? "PASS" : "FAIL"));
    }

    if (this.scenario.expectVictory && this.victory) {
      const validations = this.runValidations("PASS");
      const allPass = validations.every((v) => v.pass);
      this._finish(this.buildResult(allPass ? "PASS" : "FAIL"));
    }
  }

  buildResult(status) {
    return {
      id: this.scenario.id,
      name: this.scenario.name,
      status,
      error: this.error,
      steps: this.steps,
      phasesReached: this.phasesReached,
      victory: this.victory,
      finalPosition: { r: this.r, c: this.c, dir: this.dir },
      movements: this.movements.length,
      stderrLogs: this.stderrLogs,
      validations: this.runValidations(status),
    };
  }

  runValidations(status) {
    const validations = [];
    const mid = Math.floor(this.scenario.size / 2);

    if (this.scenario.checkInitialDistances) {
      const originDist = this.distances["0,0"];
      const centerDist = this.distances[`${mid - 1},${mid - 1}`];
      validations.push({
        name: "distância (0,0) em labirinto vazio",
        expected: 14,
        actual: originDist,
        pass: originDist === 14,
      });
      validations.push({
        name: "distância centro em labirinto vazio",
        expected: 0,
        actual: centerDist,
        pass: centerDist === 0,
      });
    }

    if (this.scenario.expectPhases) {
      for (const phase of this.scenario.expectPhases) {
        validations.push({
          name: `atingir ${phase}`,
          expected: true,
          actual: this.phasesReached.includes(phase),
          pass: this.phasesReached.includes(phase),
        });
      }
    }

    if (this.scenario.expectVictory) {
      validations.push({
        name: "vitória fase 2",
        expected: true,
        actual: this.victory,
        pass: this.victory,
      });
    }

    const allPass = validations.every((v) => v.pass);
    if (status === "PASS" && !allPass) {
      this.error = "Validações pós-execução falharam";
      return validations;
    }

    return validations;
  }
}
