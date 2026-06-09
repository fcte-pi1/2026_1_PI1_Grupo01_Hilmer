import { test, expect } from '@playwright/test';

test.describe('Testes de Sistema E2E - Histórico de Execuções', () => {

  test('Deve exibir o título da página de histórico', async ({ page }) => {
    await page.goto('/history');

    await expect(page.locator('h1', { hasText: 'Histórico de Execuções' })).toBeVisible();
  });

  test('Deve exibir o banner de dados mockados', async ({ page }) => {
    await page.goto('/history');

    await expect(page.locator('text=DADOS MOCKADOS')).toBeVisible();
  });

  test('Deve exibir a lista de execuções históricas', async ({ page }) => {
    await page.goto('/history');

    // Aguardar carregamento dos dados mockados
    const execucoes = page.locator('[class*="list"] > *');
    await expect(execucoes.first()).toBeVisible({ timeout: 5000 });

    // Verificar que há pelo menos uma execução listada
    const count = await execucoes.count();
    expect(count).toBeGreaterThan(0);
  });

  test('Deve exibir painel vazio antes de selecionar uma execução', async ({ page }) => {
    await page.goto('/history');

    // Painel lateral deve mostrar mensagem de orientação
    await expect(page.locator('text=Selecione uma execução para ver os detalhes.')).toBeVisible();
  });

  test('Deve exibir detalhes ao selecionar uma execução', async ({ page }) => {
    await page.goto('/history');

    // Aguardar lista e clicar na primeira execução
    const primeiraExecucao = page.locator('[class*="list"] > *').first();
    await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
    await primeiraExecucao.click();

    // Painel de detalhes deve mostrar o número da tentativa
    await expect(page.locator('h2', { hasText: 'Tentativa #' })).toBeVisible();
  });

  test('Deve exibir os campos de detalhe ao selecionar uma execução', async ({ page }) => {
    await page.goto('/history');

    // Selecionar primeira execução
    const primeiraExecucao = page.locator('[class*="list"] > *').first();
    await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
    await primeiraExecucao.click();

    // Verificar campos do painel de detalhes
    await expect(page.locator('text=Dimensão')).toBeVisible();
    await expect(page.locator('text=Tempo total')).toBeVisible();
    await expect(page.locator('text=Velocidade média')).toBeVisible();
    await expect(page.locator('text=Consumo total')).toBeVisible();
  });

  test('Deve exibir o status badge no painel de detalhes', async ({ page }) => {
    await page.goto('/history');

    const primeiraExecucao = page.locator('[class*="list"] > *').first();
    await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
    await primeiraExecucao.click();

    // Status badge deve estar visível no painel de detalhes
    const detailAside = page.locator('aside');
    await expect(detailAside).toBeVisible();
    const statusBadge = detailAside.locator('[class*="badge"], [class*="status"], [class*="Badge"]').first();
    await expect(statusBadge).toBeVisible();
  });

  test('Deve fechar o painel de detalhes ao clicar na execução selecionada novamente', async ({ page }) => {
    await page.goto('/history');

    const primeiraExecucao = page.locator('[class*="list"] > *').first();
    await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });

    // Selecionar
    await primeiraExecucao.click();
    await expect(page.locator('h2', { hasText: 'Tentativa #' })).toBeVisible();

    // Desselecionar (toggle)
    await primeiraExecucao.click();
    await expect(page.locator('text=Selecione uma execução para ver os detalhes.')).toBeVisible();
  });

});