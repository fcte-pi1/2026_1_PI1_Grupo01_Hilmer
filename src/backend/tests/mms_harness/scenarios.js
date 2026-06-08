/** Cenários alinhados ao roteiro_testes.tex (CT01–CT03) */

export const scenarios = [
  {
    id: "CT01",
    name: "Chegar ao Centro — distâncias iniciais",
    size: 16,
    walls: [],
    checkInitialDistances: true,
    stopAfterInitialDraw: true,
    timeoutMs: 30_000,
  },
  {
    id: "CT02",
    name: "Explorar Retorno — labirinto vazio completo",
    size: 16,
    walls: [],
    expectPhases: ["phase_0", "phase_1"],
    stopAfterPhase: "phase_1",
    timeoutMs: 300_000,
  },
  {
    id: "CT03",
    name: "Speed Run — vitória nas 3 fases",
    size: 16,
    walls: [],
    expectPhases: ["phase_0", "phase_1", "phase_2"],
    expectVictory: true,
    timeoutMs: 600_000,
  },
  {
    id: "CT04",
    name: "Labirinto com parede interna — navegação parcial",
    size: 16,
    walls: [
      { type: "horiz", r: 5, c: 5 },
      { type: "horiz", r: 5, c: 6 },
      { type: "horiz", r: 5, c: 7 },
      { type: "vert", r: 8, c: 8 },
    ],
    expectPhases: ["phase_0"],
    stopAfterPhase: "phase_0",
    timeoutMs: 300_000,
  },
];
