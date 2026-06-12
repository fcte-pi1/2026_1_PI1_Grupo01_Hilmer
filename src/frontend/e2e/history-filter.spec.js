import { test, expect } from '@playwright/test';

test.describe('History filter', () => {
  test('filters executions by maze dimension', async ({ page }) => {
    await page.goto('/history');
    await expect(page.getByText('#1')).toBeVisible({ timeout: 10_000 });
    await expect(page.getByText(/Tentativa/)).toHaveCount(6);

    await page.getByRole('button', { name: 'Filtros' }).click();
    await page.getByLabel('16x16').check();

    await expect(page.getByText(/Tentativa/)).toHaveCount(1);
    await expect(page.getByText('#4')).toBeVisible();
  });
});
