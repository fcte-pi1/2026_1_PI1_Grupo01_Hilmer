import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const frontendRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');

const result = spawnSync('npx', ['--', 'vitest', 'run', '--config', 'test/vitest.unit.config.js'], {
  cwd: frontendRoot,
  stdio: 'inherit',
  shell: true,
});

process.exit(result.status ?? 1);
