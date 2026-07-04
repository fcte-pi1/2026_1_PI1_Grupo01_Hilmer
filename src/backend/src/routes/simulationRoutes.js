/**
 * simulationRoutes.js
 *
 * Rotas que expõem as tabelas HISTORICO, TELEMETRIA e TRAJETO via HTTP.
 */

import { Router } from 'express';
import simulationService from '../services/simulationService.js';
import mouseService from '../services/mouseService.js';
import attemptService from '../services/attemptService.js';
import { clampBattery, toFiniteNumber } from '../utils/validation.js';

const router = Router();

// ─── HISTORICO ────────────────────────────────────────────────────────────────

// POST /api/historico — registra uma tentativa concluída (idempotente por espNumTentativa)
router.post('/historico', async (req, res, next) => {
  try {
    const {
      percentualBateria, velocidadeMedia,
      tempoConclusao, desafioCumprido, correnteEletrica,
      tensaoEletrica, tipoLabirinto,
    } = req.body;

    const normalizedBody = {
      ...req.body,
      percentualBateria: clampBattery(percentualBateria),
      velocidadeMedia: toFiniteNumber(velocidadeMedia, 0),
      correnteEletrica: toFiniteNumber(correnteEletrica, 0),
      tensaoEletrica: toFiniteNumber(tensaoEletrica, 0),
      tempoConclusao: tempoConclusao && !Number.isNaN(Date.parse(tempoConclusao))
        ? tempoConclusao
        : new Date().toISOString(),
      desafioCumprido: desafioCumprido === 'NAO' ? 'NAO' : 'SIM',
    };

    if (!['SIM', 'NAO'].includes(normalizedBody.desafioCumprido)) {
      return res.status(400).json({ success: false, error: "desafioCumprido deve ser 'SIM' ou 'NAO'." });
    }
    if ('numTentativa' in req.body && typeof req.body.numTentativa !== 'number') {
      return res.status(400).json({ success: false, error: 'numTentativa, quando informado, deve ser número.' });
    }
    if (!['4x4', '8x8', '16x16'].includes(tipoLabirinto)) {
      return res.status(400).json({ success: false, error: "tipoLabirinto deve ser '4x4', '8x8' ou '16x16'." });
    }
    if (normalizedBody.percentualBateria < 0 || normalizedBody.percentualBateria > 100) {
      return res.status(400).json({ success: false, error: 'percentualBateria deve estar entre 0 e 100.' });
    }

    const result = await attemptService.persistFromHttpRequest(normalizedBody);
    const statusCode = result.alreadyPersisted ? 200 : 201;

    return res.status(statusCode).json({
      success: true,
      alreadyPersisted: result.alreadyPersisted,
      data: result.record,
    });
  } catch (err) {
    next(err);
  }
});

// GET /api/historico — lista todas as tentativas
router.get('/historico', async (_req, res, next) => {
  try {
    const records = await simulationService.listarHistorico();
    return res.json({ success: true, data: records });
  } catch (err) {
    next(err);
  }
});

// GET /api/historico/:numTentativa/analise — reconstrói grid e caminhos ida/volta/ótimo
router.get('/historico/:numTentativa/analise', async (req, res, next) => {
  try {
    const numTentativa = Number(req.params.numTentativa);
    const historico = await simulationService.buscarHistoricoPorTentativa(numTentativa);

    if (!historico) {
      return res.status(404).json({ success: false, error: 'Tentativa não encontrada.' });
    }

    const trajeto = await simulationService.listarTrajetoPorTentativa(numTentativa);
    const telemetria = await simulationService.listarTelemetriaPorTentativa(numTentativa);

    if (!trajeto.length) {
      return res.status(404).json({ success: false, error: 'Trajeto não encontrado para esta tentativa.' });
    }

    const analysis = mouseService.analyzeAttempt(
      trajeto,
      telemetria,
      historico.tipolabirinto ?? historico.tipoLabirinto,
    );

    return res.json({ success: true, data: analysis });
  } catch (err) {
    next(err);
  }
});

// GET /api/historico/:numTentativa — busca tentativa específica
router.get('/historico/:numTentativa', async (req, res, next) => {
  try {
    const record = await simulationService.buscarHistoricoPorTentativa(
      Number(req.params.numTentativa)
    );
    if (!record) return res.status(404).json({ success: false, error: 'Tentativa não encontrada.' });
    return res.json({ success: true, data: record });
  } catch (err) {
    next(err);
  }
});

// ─── TELEMETRIA ───────────────────────────────────────────────────────────────

// POST /api/telemetria — salva snapshot de telemetria (chamado pelo backend ao receber da ESP32)
router.post('/telemetria', async (req, res, next) => {
  try {
    const { numTentativa, tempoColeta, bateriaAtual } = req.body;

    if (!numTentativa || !tempoColeta) {
      return res.status(400).json({ success: false, error: 'numTentativa e tempoColeta são obrigatórios.' });
    }
    if (bateriaAtual < 0 || bateriaAtual > 100) {
      return res.status(400).json({ success: false, error: 'bateriaAtual deve estar entre 0 e 100.' });
    }

    const record = await simulationService.inserirTelemetria(req.body);
    return res.status(201).json({ success: true, data: record });
  } catch (err) {
    next(err);
  }
});

// GET /api/telemetria/:numTentativa — retorna telemetria completa de uma tentativa
router.get('/telemetria/:numTentativa', async (req, res, next) => {
  try {
    const records = await simulationService.listarTelemetriaPorTentativa(
      Number(req.params.numTentativa)
    );
    return res.json({ success: true, data: records });
  } catch (err) {
    next(err);
  }
});

// ─── TRAJETO ──────────────────────────────────────────────────────────────────

// POST /api/trajeto — salva um passo do trajeto
router.post('/trajeto', async (req, res, next) => {
  try {
    const { numTentativa, passo, pos_h, pos_v, direcao } = req.body;

    if (!numTentativa || !passo || passo < 1) {
      return res.status(400).json({ success: false, error: 'numTentativa e passo (>= 1) são obrigatórios.' });
    }
    if (!['NORTE', 'SUL', 'LESTE', 'OESTE'].includes(direcao)) {
      return res.status(400).json({ success: false, error: "direcao deve ser 'NORTE', 'SUL', 'LESTE' ou 'OESTE'." });
    }

    const record = await simulationService.inserirPassoTrajeto(req.body);
    return res.status(201).json({ success: true, data: record });
  } catch (err) {
    next(err);
  }
});

// GET /api/trajeto/:numTentativa — retorna trajeto completo de uma tentativa
router.get('/trajeto/:numTentativa', async (req, res, next) => {
  try {
    const records = await simulationService.listarTrajetoPorTentativa(
      Number(req.params.numTentativa)
    );
    return res.json({ success: true, data: records });
  } catch (err) {
    next(err);
  }
});

export default router;
