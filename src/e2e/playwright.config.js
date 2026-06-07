import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
  testDir: './tests',
  fullyParallel: true,
  retries: 0,
  workers: 1, // Para testes de sistema é bom limitar concorrência dos servers
  reporter: 'html',
  use: {
    baseURL: 'http://localhost:5173',
    trace: 'on-first-retry',
    headless: true,
  },
  webServer: [
    {
      command: 'cd ../backend && node src/simulador-esp32.js',
      port: 8080,
      reuseExistingServer: !process.env.CI,
      stdout: 'pipe',
    },
    {
      command: 'cd ../backend && npm start',
      url: 'http://127.0.0.1:3001/api/health', // API Health da porta 3001
      reuseExistingServer: !process.env.CI,
      stdout: 'pipe',
    },
    {
      command: 'cd ../frontend && npm run dev',
      port: 5173,
      reuseExistingServer: !process.env.CI,
      stdout: 'pipe',
    }
  ],
  projects: [
    {
      name: 'chromium',
      use: { ...devices['Desktop Chrome'] },
    },
  ],
});
