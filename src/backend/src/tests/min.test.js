import pkg from 'pg';

const { Client } = pkg;

const client = new Client({
  host: '127.0.0.1',
  port: 5432,
  database: 'micromouse_db',
  user: 'pi1_user',
  password: 'pi1senha123',
});

try {
  await client.connect();
  console.log('CONECTOU');
} catch (e) {
  console.error(e);
}

await client.end();