import pool from '../src/database/connection.js';

try {
  const tables = await pool.query(
    `SELECT table_name FROM information_schema.tables WHERE table_schema = 'public' ORDER BY table_name`,
  );
  console.log('Tabelas:', tables.rows.map((r) => r.table_name));
} catch (error) {
  console.error('Erro:', error.message);
  process.exitCode = 1;
} finally {
  await pool.end();
}
