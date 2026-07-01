import { test, expect } from '@playwright/test';
import { seedAnalysisAttemptViaHttp } from '../../../backend/test/fixtures/analysisAttempt.js';

const backendUrl = process.env.PLAYWRIGHT_BACKEND_URL || 'http://127.0.0.1:3001';

let seededAttemptId;

test.beforeAll(async () => {
  seededAttemptId = await seedAnalysisAttemptViaHttp(backendUrl);
});

test.describe('History / analise', () => {
  test('exibe Primeiro caminho e Caminho ótimo ao selecionar tentativa', async ({ page }) => {
    await page.goto('/history');

    await expect(page.getByRole('heading', { name: 'Histórico de Tentativas' })).toBeVisible();
    await page.getByTestId(`history-row-${seededAttemptId}`).click();

    await expect(page.getByRole('heading', { name: 'Primeiro caminho' })).toBeVisible();
    await expect(page.getByRole('heading', { name: 'Caminho ótimo' })).toBeVisible();
  });
});
