/**
 * mouseController.js
 */

import mouseService from '../services/mouseService.js';

function calculateNextMove(req, res, next) {
  try {
    const { currentPosition, mazeMatrix } = req.body;

    if (!currentPosition || !mazeMatrix) {
      return res.status(400).json({
        error: 'currentPosition e mazeMatrix são obrigatórios.',
      });
    }

    if (typeof currentPosition.row !== 'number' || typeof currentPosition.col !== 'number') {
      return res.status(400).json({
        error: 'currentPosition deve ter campos { row: number, col: number }.',
      });
    }

    const decision = mouseService.processFloodFill(currentPosition, mazeMatrix);

    return res.status(200).json({
      success: true,
      nextPosition: decision.nextPosition,
      action: decision.action,
    });
  } catch (error) {
    next(error);
  }
}

export default { calculateNextMove };