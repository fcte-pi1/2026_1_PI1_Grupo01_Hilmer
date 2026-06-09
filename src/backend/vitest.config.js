import { defineConfig } from 'vitest/config';

export default defineConfig({
  test: {
    fileParallelism: false,
    include: ['tests/**/*.test.js', 'tests/*.test.js']
  }
});
