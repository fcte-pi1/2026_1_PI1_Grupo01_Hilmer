import { test, expect } from '@playwright/test';

test.describe('Testes de Sistema E2E - Nova Tentativa', () => {

  test('Deve exibir os elementos principais da página de nova tentativa', async ({ page }) => {
    await page.goto('/new-attempt');

    // Verificar instrução de seleção de tamanho
    await expect(page.locator('text=Escolha o tamanho do labirinto:')).toBeVisible();

    // Verificar botão de voltar
    await expect(page.locator('button', { hasText: 'Voltar' })).toBeVisible();
  });

  test('Deve exibir as opções de tamanho do labirinto', async ({ page }) => {
    await page.goto('/new-attempt');

    // Verificar que todos os tamanhos disponíveis estão presentes
    for (const size of [10, 12, 14, 16, 18, 20]) {
      await expect(page.locator('button', { hasText: `${size}×${size}` })).toBeVisible();
    }
  });

  test('Botão "Ativar rato" deve estar desabilitado sem tamanho selecionado', async ({ page }) => {
    await page.goto('/new-attempt');

    const activateBtn = page.locator('button', { hasText: 'Ativar rato' });
    await expect(activateBtn).toBeVisible();
    await expect(activateBtn).toBeDisabled();
  });

  test('Deve habilitar o botão "Ativar rato" após selecionar um tamanho', async ({ page }) => {
    await page.goto('/new-attempt');

    // Selecionar um tamanho de labirinto
    await page.locator('button', { hasText: '16×16' }).click();

    // Botão deve estar habilitado agora
    const activateBtn = page.locator('button', { hasText: 'Ativar rato' });
    await expect(activateBtn).toBeEnabled();
  });

  test('Deve navegar para /dashboard ao ativar com tamanho selecionado', async ({ page }) => {
    await page.goto('/new-attempt');

    // Selecionar tamanho e ativar
    await page.locator('button', { hasText: '16×16' }).click();
    await page.locator('button', { hasText: 'Ativar rato' }).click();

    await expect(page).toHaveURL('/dashboard');
  });

  test('Deve exibir as métricas laterais com valores padrão', async ({ page }) => {
    await page.goto('/new-attempt');

    // Verificar labels das métricas na sidebar
    await expect(page.locator('text=Dimensão')).toBeVisible();
    await expect(page.locator('text=Consumo de bateria')).toBeVisible();
    await expect(page.locator('text=Velocidade')).toBeVisible();
    await expect(page.locator('text=Tempo')).toBeVisible();

    // Verificar valor padrão de tempo
    await expect(page.locator('text=00:00:00')).toBeVisible();
  });

  test('Deve voltar para a página inicial ao clicar em "Voltar"', async ({ page }) => {
    await page.goto('/new-attempt');

    await page.locator('button', { hasText: 'Voltar' }).click();

    await expect(page).toHaveURL('/');
  });

});
