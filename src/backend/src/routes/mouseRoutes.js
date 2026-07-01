/**
 * mouseRoutes.js
 */

import { Router } from 'express';
import mouseController from '../controllers/mouseController.js';

const router = Router();

// POST /api/mouse/next-move — calcula próximo movimento via FloodFill
router.post('/next-move', mouseController.calculateNextMove);

export default router;