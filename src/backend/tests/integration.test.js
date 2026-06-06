import { describe, it, expect, beforeAll, afterAll } from 'vitest';
import { WebSocketServer, WebSocket } from 'ws';
import { server, wssReact, connectToESP32 } from '../src/server.js';

describe('Teste de Integração: Broker WebSocket', () => {
  let mockESP32Server;
  let clienteReactFalso;

  beforeAll(() => {
    
    process.env.ESP32_WS_URL = 'ws://127.0.0.1:8080';
    mockESP32Server = new WebSocketServer({ port: 8080 });
    
    connectToESP32();
  });

  afterAll(() => {
    
    mockESP32Server.close();
    if (clienteReactFalso) clienteReactFalso.close();
    server.close();
    wssReact.close();
  });

  it('O Backend deve repassar a telemetria da ESP32 para o Frontend', () => {
    
    return new Promise((resolve, reject) => {
      
      const telemetriaFalsa = { 
        tipoLabirinto: "16x16", 
        bateriaConsumo: 85, 
        desafioCumprido: "S" 
      };
      
      clienteReactFalso = new WebSocket('ws://127.0.0.1:3001');
      
      clienteReactFalso.on('open', () => {
        
        mockESP32Server.clients.forEach(client => {
          client.send(JSON.stringify(telemetriaFalsa));
        });
      });
      
      clienteReactFalso.on('message', (data) => {
        try {
          const recebido = JSON.parse(data);
          
          expect(recebido.tipoLabirinto).toBe("16x16");
          expect(recebido.bateriaConsumo).toBe(85);
          expect(recebido.desafioCumprido).toBe("S");
          
          resolve(); 
        } catch (erro) {
          reject(erro); 
        }
      });
      
      setTimeout(() => reject(new Error('Timeout: O Backend não repassou a mensagem')), 2000);
    });
  });

});