import { afterAll, beforeAll, describe, expect, it } from 'vitest';
import { server } from '../src/server.js';
import pool from '../src/database/connection.js';

const uniqueSeed = Date.now();
const historicoTentativa = Number(String(uniqueSeed).slice(-8));
const telemetriaTentativa = historicoTentativa + 1;
const trajetoTentativa = historicoTentativa + 2;

describe.sequential('Integração das rotas de simulação', () => {
  beforeAll(async () => {
    await ensureServerReady();
  });

  afterAll(async () => {
    await cleanupTentativas([historicoTentativa, telemetriaTentativa, trajetoTentativa]);
    await new Promise((resolve) => server.close(resolve));
  });

  it('cria e busca um HISTORICO real via HTTP', async () => {
    const payload = {
      numTentativa: historicoTentativa,
      percentualBateria: 92.5,
      velocidadeMedia: 0.67,
      tempoConclusao: new Date().toISOString(),
      desafioCumprido: 'SIM',
      correnteEletrica: 1.4,
      tensaoEletrica: 7.4,
      tipoLabirinto: '16x16',
    };

    const postResponse = await fetch('http://127.0.0.1:3001/api/historico', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    });

    expect(postResponse.status).toBe(201);
    const postData = await postResponse.json();
    expect(postData.success).toBe(true);
    expect(postData.data.numtentativa).toBe(historicoTentativa);

    const getResponse = await fetch(`http://127.0.0.1:3001/api/historico/${historicoTentativa}`);
    expect(getResponse.status).toBe(200);

    const getData = await getResponse.json();
    expect(getData.data.numtentativa).toBe(historicoTentativa);
  });

  it('rejeita HISTORICO inválido com 400', async () => {
    const response = await fetch('http://127.0.0.1:3001/api/historico', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        numTentativa: 0,
        percentualBateria: 200,
        velocidadeMedia: 0.1,
        tempoConclusao: new Date().toISOString(),
        desafioCumprido: 'TALVEZ',
        correnteEletrica: 1.0,
        tensaoEletrica: 7.2,
        tipoLabirinto: '32x32',
      }),
    });

    expect(response.status).toBe(400);
  });

  it('cria, lista e busca TELEMETRIA real via HTTP', async () => {
    await createHistoricoParaTentativa(telemetriaTentativa);

    const payload = {
      numTentativa: telemetriaTentativa,
      tempoColeta: new Date().toISOString(),
      tensaoRecente: 7.2,
      correnteRecente: 1.0,
      posHRecente: 4,
      posVRecente: 5,
      velocidadeAtual: 0.55,
      bateriaAtual: 81.5,
      tensaoAtual: 7.3,
      sensorCor: '#A1B2C3',
      sensorEsquerda: 10.5,
      sensorDireita: 11.5,
      sensorFrontal: 12.5,
    };

    const postResponse = await fetch('http://127.0.0.1:3001/api/telemetria', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload),
    });

    expect(postResponse.status).toBe(201);

    const listResponse = await fetch(`http://127.0.0.1:3001/api/telemetria/${telemetriaTentativa}`);
    expect(listResponse.status).toBe(200);

    const listData = await listResponse.json();
    expect(listData.data).toHaveLength(1);
    expect(listData.data[0].numtentativa).toBe(telemetriaTentativa);
  });

  it('cria e lista TRAJETO real via HTTP', async () => {
    await createHistoricoParaTentativa(trajetoTentativa);

    for (const passo of [1, 2, 3]) {
      const response = await fetch('http://127.0.0.1:3001/api/trajeto', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          numTentativa: trajetoTentativa,
          passo,
          pos_h: passo - 1,
          pos_v: passo,
          direcao: passo === 1 ? 'NORTE' : passo === 2 ? 'LESTE' : 'SUL',
        }),
      });

      expect(response.status).toBe(201);
    }

    const listResponse = await fetch(`http://127.0.0.1:3001/api/trajeto/${trajetoTentativa}`);
    expect(listResponse.status).toBe(200);

    const listData = await listResponse.json();
    expect(listData.data).toHaveLength(3);
    expect(listData.data.map((item) => item.passo)).toEqual([1, 2, 3]);
  });
});

async function createHistoricoParaTentativa(numTentativa) {
  const response = await fetch('http://127.0.0.1:3001/api/historico', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({
      numTentativa,
      percentualBateria: 90,
      velocidadeMedia: 0.5,
      tempoConclusao: new Date().toISOString(),
      desafioCumprido: 'SIM',
      correnteEletrica: 1.2,
      tensaoEletrica: 7.4,
      tipoLabirinto: '16x16',
    }),
  });

  expect(response.status).toBe(201);
}

async function cleanupTentativas(tentativas) {
  for (const numTentativa of tentativas) {
    await pool.query('DELETE FROM HISTORICO WHERE numTentativa = $1', [numTentativa]);
  }
}

async function ensureServerReady() {
  const deadline = Date.now() + 4000;

  while (Date.now() < deadline) {
    try {
      const response = await fetch('http://127.0.0.1:3001/api/health');
      if (response.ok) {
        return;
      }
    } catch {
      // tenta novamente
    }

    await new Promise((resolve) => setTimeout(resolve, 100));
  }

  throw new Error('Servidor HTTP não ficou pronto a tempo.');
}