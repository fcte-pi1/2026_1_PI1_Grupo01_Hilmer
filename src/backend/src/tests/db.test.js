// =============================================================
// TESTES UNITÁRIOS - Banco de Dados Micromouse PI1
// Grupo 01 - Hilmer 2026/1
// db.test.js
import { Pool } from 'pg';
import dotenv from 'dotenv';
import path from 'path';
import { fileURLToPath } from 'url';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);

dotenv.config({
  path: path.resolve(__dirname, '../../.env')
});

console.log(JSON.stringify(process.env.DB_PASSWORD));

console.log('ENV carregado:', path.resolve(__dirname, '../../.env'));

const pool = new Pool({
  host: 'localhost',
  port: 5432,
  database: 'micromouse_db',
  user: 'pi1_user',
  password: 'pi1senha123',
});

console.log({
  host: process.env.DB_HOST,
  port: process.env.DB_PORT,
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD
});

let passou = 0;
let falhou = 0;

function ok(nome) {
  console.log(`  PASSOU: ${nome}`);
  passou++;
}

function fail(nome, erro) {
  console.log(`  X FALHOU: ${nome}`);
  console.log(`     Motivo: ${erro.message}`);
  falhou++;
}


async function testarConexao() {
  console.log('\n [1] Conexao com o banco');
  try {
    const res = await pool.query('SELECT NOW()');
    if (res.rows.length > 0) ok('Conexao estabelecida com sucesso');
  } catch (e) {
    fail('Conectar ao PostgreSQL', e);
            console.log({
        message: e.message,
        code: e.code,
        detail: e.detail,
        stack: e.stack
        });
  }
}

async function testarTabelasExistem() {
  console.log('\n [2] Existência das tabelas');
  const tabelas = ['historico', 'telemetria', 'trajeto'];

  for (const tabela of tabelas) {
    try {
      await pool.query(`SELECT 1 FROM ${tabela} LIMIT 1`);
      ok(`Tabela "${tabela}" existe`);
    } catch (e) {
      fail(`Tabela "${tabela}" existe`, e);
    }
  }
}

async function testarInsertHistorico() {
  console.log('\n [3] INSERT em HISTORICO');
  try {
    const res = await pool.query(`
      INSERT INTO HISTORICO
        (percentualBateria, velocidadeMedia, tempoConclusao, desafioCumprido,
         correnteEletrica, tensaoEletrica, tipoLabirinto)
      VALUES
        (85.5, 0.45, NOW(), 'SIM', 1.2, 7.4, 'PADRAO')
      RETURNING numTentativa
    `);
    const id = res.rows[0].numtentativa;
    if (id) ok(`HISTORICO inserido com numTentativa = ${id}`);
    return id;
  } catch (e) {
    fail('INSERT em HISTORICO', e);
    return null;
  }
}

async function testarInsertTelemetria(numTentativa) {
  console.log('\n [4] INSERT em TELEMETRIA');
  if (!numTentativa) {
    console.log('  Pulado (numTentativa inválido)');
    return;
  }
  try {
    await pool.query(`
      INSERT INTO TELEMETRIA
        (numTentativa, tempoColeta, tensaoRecente, correnteRecente,
         posHRecente, posVRecente, velocidadeAtual, bateriaAtual,
         tensaoAtual, sensorCor, sensorEsquerda, sensorDireita, sensorFrontal)
      VALUES
        ($1, NOW(), 7.3, 1.1, 0, 0, 0.5, 88.0, 7.4, 'BRANCO', 15.0, 20.0, 5.0)
    `, [numTentativa]);
    ok('TELEMETRIA inserida e vinculada ao HISTORICO');
  } catch (e) {
    fail('INSERT em TELEMETRIA', e);
  }
}

async function testarInsertTrajeto(numTentativa) {
  console.log('\n [5] INSERT em TRAJETO');
  if (!numTentativa) {
    console.log('  Pulado (numTentativa inválido)');
    return;
  }
  try {
    await pool.query(`
      INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
      VALUES
        ($1, 1, 0, 0, 'NORTE'),
        ($1, 2, 0, 1, 'NORTE'),
        ($1, 3, 1, 1, 'LESTE')
    `, [numTentativa]);
    ok('3 passos de TRAJETO inseridos com sucesso');
  } catch (e) {
    fail('INSERT em TRAJETO', e);
  }
}

async function testarChaveEstrangeiraInvalida() {
  console.log('\n [6] Integridade referencial (FK inválida deve falhar)');
  try {
    await pool.query(`
      INSERT INTO TELEMETRIA
        (numTentativa, tempoColeta, tensaoRecente, correnteRecente,
         posHRecente, posVRecente, velocidadeAtual, bateriaAtual, tensaoAtual)
      VALUES
        (99999, NOW(), 7.3, 1.1, 0, 0, 0.5, 88.0, 7.4)
    `);
    fail('FK inválida deveria ter sido rejeitada', new Error('INSERT não falhou como esperado'));
  } catch (e) {
    ok('FK inválida foi corretamente rejeitada pelo banco');
  }
}

async function testarEnumDirecaoInvalido(numTentativa) {
  console.log('\n [7] CHECK constraint (direção inválida deve falhar)');
  if (!numTentativa) {
    console.log('  Pulado (numTentativa inválido)');
    return;
  }
  try {
    await pool.query(`
      INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
      VALUES ($1, 99, 0, 0, 'DIAGONAL')
    `, [numTentativa]);
    fail('Direção inválida deveria ter sido rejeitada', new Error('INSERT não falhou como esperado'));
  } catch (e) {
    ok('Direção inválida foi corretamente rejeitada pelo CHECK constraint');
  }
}

async function testarPKCompostaTrajetoUnica(numTentativa) {
  console.log('\n [8] PK composta TRAJETO (passo duplicado deve falhar)');
  if (!numTentativa) {
    console.log('  Pulado (numTentativa inválido)');
    return;
  }
  try {
    // passo 1 já foi inserido no teste [5]
    await pool.query(`
      INSERT INTO TRAJETO (numTentativa, passo, pos_h, pos_v, direcao)
      VALUES ($1, 1, 5, 5, 'SUL')
    `, [numTentativa]);
    fail('PK duplicada deveria ter sido rejeitada', new Error('INSERT não falhou como esperado'));
  } catch (e) {
    ok('PK composta (numTentativa, passo) duplicada foi corretamente rejeitada');
  }
}

async function testarCascadeDelete(numTentativa) {
  console.log('\n [9] CASCADE DELETE');
  if (!numTentativa) {
    console.log('  Pulado (numTentativa inválido)');
    return;
  }
  try {
    await pool.query('DELETE FROM HISTORICO WHERE numTentativa = $1', [numTentativa]);

    const tel = await pool.query('SELECT 1 FROM TELEMETRIA WHERE numTentativa = $1', [numTentativa]);
    const traj = await pool.query('SELECT 1 FROM TRAJETO WHERE numTentativa = $1', [numTentativa]);

    if (tel.rows.length === 0 && traj.rows.length === 0) {
      ok('DELETE em HISTORICO removeu TELEMETRIA e TRAJETO em cascata');
    } else {
      fail('CASCADE DELETE', new Error('Registros filhos não foram removidos'));
    }
  } catch (e) {
    fail('CASCADE DELETE', e);
  }
}

// ─────────────────────────────────────────────────
// RUNNER
// ─────────────────────────────────────────────────

async function rodarTestes() {
  console.log('='.repeat(55));
  console.log('  TESTES - Banco Micromouse PI1');
  console.log('='.repeat(55));

  await testarConexao();
  await testarTabelasExistem();
  const numTentativa = await testarInsertHistorico();
  await testarInsertTelemetria(numTentativa);
  await testarInsertTrajeto(numTentativa);
  await testarChaveEstrangeiraInvalida();
  await testarEnumDirecaoInvalido(numTentativa);
  await testarPKCompostaTrajetoUnica(numTentativa);
  await testarCascadeDelete(numTentativa);

  console.log('\n' + '='.repeat(55));
  console.log(`  Resultado: ${passou} passaram | ${falhou} falharam`);
  console.log('='.repeat(55) + '\n');

  await pool.end();
  process.exit(falhou > 0 ? 1 : 0);
}

rodarTestes();