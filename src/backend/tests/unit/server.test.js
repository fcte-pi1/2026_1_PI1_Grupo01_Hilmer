import { describe, it, expect, afterAll } from 'vitest';
import { server, wssReact } from '../src/server.js';
import WebSocket from 'ws'; 

describe('Testes da API HTTP e WebSocket (Vitest)', () => {

  afterAll(() => {
    server.close();
    wssReact.close();
  });
  

  it('A rota /api/health deve retornar status 200', async () => {
    const response = await fetch('http://127.0.0.1:3001/api/health');
    expect(response.status).toBe(200);
    const data = await response.json();
    expect(data.status).toBe('online');
  });

  it('Deve retornar erro 404 para rotas falsas', async () => {
    const response = await fetch('http://127.0.0.1:3001/api/rota-falsa');
    expect(response.status).toBe(404);
  });

  

  it('A rota /api/status deve retornar as definições do projeto', async () => {
    const response = await fetch('http://127.0.0.1:3001/api/status');
    expect(response.status).toBe(200);
    
    const data = await response.json();
    
    expect(data).toHaveProperty('frontend');
    expect(data).toHaveProperty('backend');
    expect(data).toHaveProperty('database');
  });

  it('Deve responder corretamente às requisições OPTIONS (CORS)', async () => {
    
    const response = await fetch('http://127.0.0.1:3001/api/health', {
      method: 'OPTIONS'
    });
    
    expect(response.status).toBe(204); 
    
    expect(response.headers.get('access-control-allow-origin')).not.toBeNull();
  });

  

  it('Deve aceitar conexões WebSocket na porta 3001', () => {
    
    return new Promise((resolve, reject) => {
      
      const ws = new WebSocket('ws://127.0.0.1:3001');
      
      ws.on('open', () => {
        
        expect(ws.readyState).toBe(WebSocket.OPEN);
        ws.close(); 
        resolve();  
      });

      ws.on('error', (err) => {
        reject(err); 
      });
    });
  });

});