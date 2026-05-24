const express = require('express');
const router = express.Router();
const mouseController = require('../controllers/mouseController');

// Rota para calcular o próximo movimento do Micromouse
router.post('/next-move', mouseController.calculateNextMove);

module.exports = router;