/**
 * esp32Routes.js
 *
 * Expõe o estado real da conexão do backend com a ESP32 (distinto do
 * WebSocket do broker com o frontend, que fica de pé independente do
 * robô estar conectado) e permite forçar uma reconexão manual.
 *
 * getStatus/reconnect são injetados por quem monta as rotas (server.js,
 * dono de fato do socket da ESP32) para evitar um import circular.
 */

import { Router } from 'express';

export default function createEsp32Routes({ getStatus, reconnect }) {
  const router = Router();

  router.get('/status', (_req, res) => {
    res.json({ success: true, data: getStatus() });
  });

  router.post('/reconnect', (_req, res) => {
    reconnect();
    res.json({ success: true, data: getStatus() });
  });

  return router;
}
