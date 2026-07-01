import { afterAll, describe, expect, it } from 'vitest';
import { Pool } from 'pg';
import dotenv from 'dotenv';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

dotenv.config({
  path: path.resolve(__dirname, '../../.env'),
});

const pool = new Pool({
  host: process.env.DB_HOST || 'localhost',
  port: Number(process.env.DB_PORT || 5432),
  database: process.env.DB_NAME || 'micromouse_db',
  user: process.env.DB_USER || 'pi1_user',
  password: process.env.DB_PASSWORD || 'pi1senha123',
});

let numTentativa;

describe.sequential('Banco Micromouse PI1', () => {
  afterAll(async () => {
    if (numTentativa) {
      await pool.query('DELETE FROM HISTORICO WHERE numTentativa = $1', [numTentativa]).catch(() => {});
    }

    await pool.end();
  });

  it('conecta ao PostgreSQL', async () => {
    const result = await pool.query('SELECT NOW()');
    expect(result.rows).toHaveLength(1);
  });

  it.each(['historico', 'telemetria', 'trajeto'])('encontra a tabela %s', async (tabela) => {
    const result = await pool.query(`SELECT 1 FROM ${tabela} LIMIT 1`);
    expect(result).toBeDefined();
  });

  it('insere HISTORICO e captura numTentativa', async () => {
    numTentativa = Math.floor(Date.now() / 1000) % 2000000000;

    const result = await pool.query(`
      INSERT INTO HISTORICO
        (numTentativa, percentualBateria, velocidadeMedia, tempoConclusao,
         desafioCumprido, correnteEletrica, tensaoEletrica, tipoLabirinto)
      VALUES
        ($1, 85.5, 0.45, NOW(), 'SIM', 1.2, 7.4, '16x16')
      RETURNING numTentativa
    `, [numTentativa]);

    numTentativa = result.rows[0].numtentativa;
    expect(numTentativa).toBeTypeOf('number');
  });

  it('insere TELEMETRIA vinculada ao HISTORICO', async () => {
    const result = await pool.query(
      `INSERT INTO TELEMETRIA
        (numTentativa, tempoColeta, tensaoRecente, correnteRecente,
         posHRecente, posVRecente, velocidadeAtual, bateriaAtual,
         tensaoAtual, sensorCor, sensorEsquerda, sensorDireita, sensorFrontal)
       VALUES
        ($1, NOW(), 7.3, 1.1, 0, 0, 0.5, 88.0, 7.4, '#FFFFFF', 15.0, 20.0, 5.0)
      RETURNING *`,
      [numTentativa]
    );

    expect(result.rows[0].numtentativa).toBe(numTentativa);
  });

  it('insere três passos em TRAJETO', async () => {
    const result = await pool.query(
      `INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
       VALUES
        ($1, 1, 0, 0, 'NORTE'),
        ($1, 2, 0, 1, 'NORTE'),
        ($1, 3, 1, 1, 'LESTE')
       RETURNING *`,
      [numTentativa]
    );

    expect(result.rows).toHaveLength(3);
  });

  it('rejeita FK inválida em TELEMETRIA', async () => {
    await expect(
      pool.query(`
        INSERT INTO TELEMETRIA
          (numTentativa, tempoColeta, tensaoRecente, correnteRecente,
           posHRecente, posVRecente, velocidadeAtual, bateriaAtual, tensaoAtual)
        VALUES
          (99999, NOW(), 7.3, 1.1, 0, 0, 0.5, 88.0, 7.4)
      `)
    ).rejects.toMatchObject({ code: '23503' });
  });

  it('rejeita direção inválida em TRAJETO', async () => {
    await expect(
      pool.query(`
        INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
        VALUES ($1, 99, 0, 0, 'XYZ')
      `, [numTentativa])
    ).rejects.toMatchObject({ code: '23514' });
  });

  it('rejeita passo duplicado na PK composta de TRAJETO', async () => {
    await expect(
      pool.query(`
        INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
        VALUES ($1, 1, 5, 5, 'SUL')
      `, [numTentativa])
    ).rejects.toMatchObject({ code: '23505' });
  });

  it('remove filhos em cascata ao apagar HISTORICO', async () => {
    await pool.query('DELETE FROM HISTORICO WHERE numTentativa = $1', [numTentativa]);

    const telemetria = await pool.query('SELECT 1 FROM TELEMETRIA WHERE numTentativa = $1', [numTentativa]);
    const trajeto = await pool.query('SELECT 1 FROM TRAJETO WHERE numTentativa = $1', [numTentativa]);

    expect(telemetria.rows).toHaveLength(0);
    expect(trajeto.rows).toHaveLength(0);

    numTentativa = undefined;
  });
});
