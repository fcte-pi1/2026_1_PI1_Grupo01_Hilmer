const mouseService = require('../services/mouseService');

exports.calculateNextMove = (req, res, next) => {
  try {
    const { currentPosition, mazeMatrix } = req.body;

    // Validação básica de entrada de dados
    if (!currentPosition || !mazeMatrix) {
      return res.status(400).json({ error: "Posição atual e matriz do labirinto são obrigatórias." });
    }

    // Processa a lógica através da camada de serviço
    const decision = mouseService.processFloodFill(currentPosition, mazeMatrix);

    return res.status(200).json({
      success: true,
      nextPosition: decision.nextPosition,
      action: decision.action
    });
  } catch (error) {
    next(error);
  }
}