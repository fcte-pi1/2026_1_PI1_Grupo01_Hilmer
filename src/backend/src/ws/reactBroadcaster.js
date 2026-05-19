/**
 * IMPORTANTE CONFIRMAR SE ESTE ARQUIVO É NECESSÁRIO NO FRONTEND; SE NÃO FOR, REMOVER E SUBSTITUIR PELA IMPLEMENTAÇÃO DO WS NO FRONTEND.
 * 
 * Gerencia o WebSocketServer voltado ao frontend React.
 * Aceita conexões, envia snapshot inicial e distribui atualizações de estado.
 */

import { WebSocketServer, WebSocket } from 'ws';

export let wssReact = null;

/**
 * Inicializa o WebSocketServer acoplado ao servidor HTTP.
 *
 * @param {import('node:http').Server} httpServer
 * @param {() => object|null} getLatestState  — getter do estado atual, injetado pelo server.js
 */
export function initReactBroadcaster(httpServer, getLatestState = () => null) {
  wssReact = new WebSocketServer({ server: httpServer });

  wssReact.on('connection', (ws) => {
    console.log('[backend/src/ws] Frontend React conectado.');

    const latest = getLatestState();
    if (latest) ws.send(JSON.stringify({ type: 'maze_state', payload: latest }));

    ws.on('close', () => console.log('[backend/src/ws] Frontend React desconectado.'));
  });

  return wssReact;
}

/**
 * Distribui um estado processado para todos os clientes React conectados.
 *
 * @param {object} mazeState
 */
export function broadcastMazeState(mazeState) {
  if (!wssReact) return;
  const message = JSON.stringify({ type: 'maze_state', payload: mazeState });
  wssReact.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) client.send(message);
  });
}