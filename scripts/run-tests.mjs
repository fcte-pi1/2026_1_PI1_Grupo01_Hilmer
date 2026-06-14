/**
 * Executa testes ignorando argumentos extras passados pelo npm no Windows
 * (ex.: `npm run test:unit # comentario` não quebra vitest/playwright).
 */
import { spawnSync } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const repoRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const suite = process.argv[2];

const suites = {
  unit: [
    {
      cwd: path.join(repoRoot, 'src/backend'),
      args: ['vitest', 'run', '--config', 'test/vitest.unit.config.js'],
    },
    {
      cwd: path.join(repoRoot, 'src/frontend'),
      args: ['vitest', 'run', '--config', 'test/vitest.unit.config.js'],
    },
  ],
  integration: [
    {
      cwd: path.join(repoRoot, 'src/backend'),
      args: ['vitest', 'run', '--config', 'test/vitest.integration.config.js'],
    },
  ],
  e2e: [
    {
      cwd: path.join(repoRoot, 'src/frontend'),
      args: ['playwright', 'test', '--config', 'test/playwright.config.js'],
    },
  ],
};

const steps = suites[suite];

if (!steps) {
  console.error(`Suite desconhecida: ${suite ?? '(vazia)'}`);
  console.error('Use: unit | integration | e2e');
  process.exit(1);
}

for (const step of steps) {
  const result = spawnSync('npx', ['--', ...step.args], {
    cwd: step.cwd,
    stdio: 'inherit',
    shell: true,
  });

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}
