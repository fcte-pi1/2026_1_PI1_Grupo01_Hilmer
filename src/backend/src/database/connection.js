//const { Pool } = require('pg');
import { Pool } from 'pg';
import dotenv from 'dotenv';

dotenv.config();

const host = process.env.DB_HOST || 'localhost';

function shouldUseSsl() {
  if (process.env.DB_SSL === 'false') return false;
  if (process.env.DB_SSL === 'true' || process.env.DB_SSL === 'require') return true;
  return !['localhost', '127.0.0.1'].includes(host);
}

const pool = new Pool({
  host,
  port: Number(process.env.DB_PORT || 5432),
  database: process.env.DB_NAME,
  user: process.env.DB_USER,
  password: process.env.DB_PASSWORD,
  ...(shouldUseSsl() ? { ssl: { rejectUnauthorized: false } } : {}),
});

// module.exports = pool;
export default pool;