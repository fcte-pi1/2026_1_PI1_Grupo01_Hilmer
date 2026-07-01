import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    include: ['test/unit/**/*.unit.test.js'],
    fileParallelism: false,
    env: {
      NODE_ENV: 'test',
    },
  },
});
