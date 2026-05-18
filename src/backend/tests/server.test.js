import { describe, it, expect, afterAll } from 'vitest';
import { server, wssReact } from '../src/server.js';
import WebSocket from 'ws'; // Importamos o ws para simular o React conectando

describe('Testes da API HTTP e WebSocket (Vitest)', () => {

  afterAll(() => {
    server.close();
    wssReact.close();
  });

  // --- TESTES DE ROTAS HTTP EXISTENTES ---

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

  // --- NOVOS TESTES HTTP (STATUS E CORS) ---

  it('A rota /api/status deve retornar as definições do projeto', async () => {
    const response = await fetch('http://127.0.0.1:3001/api/status');
    expect(response.status).toBe(200);
    
    const data = await response.json();
    // Verifica se as chaves existem no JSON de resposta
    expect(data).toHaveProperty('frontend');
    expect(data).toHaveProperty('backend');
    expect(data).toHaveProperty('database');
  });

  it('Deve responder corretamente às requisições OPTIONS (CORS)', async () => {
    // Simulamos a requisição invisível que o navegador faz
    const response = await fetch('http://127.0.0.1:3001/api/health', {
      method: 'OPTIONS'
    });
    
    expect(response.status).toBe(204); // 204 No Content é o padrão para OPTIONS
    // Verifica se os cabeçalhos de segurança (CORS) estão presentes
    expect(response.headers.get('access-control-allow-origin')).not.toBeNull();
  });

  // --- NOVO TESTE DE WEBSOCKET ---

  it('Deve aceitar conexões WebSocket na porta 3001', () => {
    // Como WebSockets funcionam baseados em eventos, usamos uma Promise para o teste aguardar
    return new Promise((resolve, reject) => {
      // Tenta conectar no backend imitando o React
      const ws = new WebSocket('ws://127.0.0.1:3001');
      
      ws.on('open', () => {
        // Se o evento 'open' disparar, a conexão foi um sucesso!
        expect(ws.readyState).toBe(WebSocket.OPEN);
        ws.close(); // Fecha a conexão de teste
        resolve();  // Avisa ao Vitest que o teste passou
      });

      ws.on('error', (err) => {
        reject(err); // Se der erro, avisa ao Vitest que o teste falhou
      });
    });
  });

});