import { test, expect } from '@playwright/test';

test.describe('Testes de Sistema E2E - Home', () => {

  test('Deve exibir os elementos principais da página inicial', async ({ page }) => {
    // Acessar a página inicial
    await page.goto('/');

    // Verificar badge de identidade do projeto
    await expect(page.locator('text=MICROMOUSE')).toBeVisible();

    // Verificar título principal
    await expect(page.locator('h1')).toBeVisible();

    // Verificar subtítulo descritivo
    await expect(page.locator('text=Controle e monitore o percurso do rato em tempo real')).toBeVisible();
  });

  test('Deve exibir os dois botões de ação na página inicial', async ({ page }) => {
    await page.goto('/');

    // Verificar botão primário de nova tentativa
    const btnNova = page.locator('button', { hasText: 'Nova tentativa' });
    await expect(btnNova).toBeVisible();

    // Verificar botão secundário de histórico
    const btnHistorico = page.locator('button', { hasText: 'Consultar tentativas' });
    await expect(btnHistorico).toBeVisible();
  });

  test('Deve navegar para /new-attempt ao clicar em "Nova tentativa"', async ({ page }) => {
    await page.goto('/');

    await page.locator('button', { hasText: 'Nova tentativa' }).click();

    await expect(page).toHaveURL('/new-attempt');
  });

  test('Deve navegar para /history ao clicar em "Consultar tentativas"', async ({ page }) => {
    await page.goto('/');

    await page.locator('button', { hasText: 'Consultar tentativas' }).click();

    await expect(page).toHaveURL('/history');
  });

});
