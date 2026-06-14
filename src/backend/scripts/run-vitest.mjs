import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const backendRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const config = process.argv[2];

const configs = {
  unit: 'test/vitest.unit.config.js',
  integration: 'test/vitest.integration.config.js',
};

const configFile = configs[config];

if (!configFile) {
  console.error(`Config desconhecida: ${config ?? '(vazia)'}`);
  process.exit(1);
}

const result = spawnSync('npx', ['--', 'vitest', 'run', '--config', configFile], {
  cwd: backendRoot,
  stdio: 'inherit',
  shell: true,
});

process.exit(result.status ?? 1);
