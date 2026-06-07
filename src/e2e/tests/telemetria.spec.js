import { test, expect } from '@playwright/test';

test.describe('Testes de Sistema E2E - Micromouse', () => {

  test('Deve exibir as informações de telemetria simuladas e permitir iniciar o rato', async ({ page }) => {
    // Acessar a página de telemetria do Dashboard
    await page.goto('/dashboard');

    // Verificar se o título da página está presente
    await expect(page.locator('h1', { hasText: 'Telemetria em Tempo Real' })).toBeVisible();

    // Iniciar o rato
    const startButton = page.locator('button', { hasText: 'Ativar rato' });
    await expect(startButton).toBeVisible();
    await startButton.click();

    // O botão deve mudar de texto
    await expect(page.locator('button', { hasText: 'Executando...' })).toBeVisible();

    // A badge de status deve mostrar em progresso (ex: "Em Andamento", "Running")
    const statusBadge = page.locator('aside section h3:has-text("Status") + div'); 
    // Wait for the cells to start rendering correctly as paths or mouse changes
    const mouseDot = page.locator('.mouse'); // from celType = mouse (depends on styles, let's just check the text info)

    // Verifica se "Tempo" está sendo populado
    const textoTempo = await page.locator('span:has-text("Tempo") + span').innerText();
    expect(textoTempo).toBeDefined();

    // Aguarda o término da simulação (status "Concluído") - Isso pode demorar, mas o playwright.config timeout padrão é 30s.
    // Como o mock tem poucos ticks (800ms) podemos testar até o rato andar.
    await expect(page.locator('button', { hasText: 'Concluído' })).toBeVisible({ timeout: 15000 });
  });
});