# Instructions

- Following Playwright test failed.
- Explain why, be concise, respect Playwright best practices.
- Provide a snippet of code with the fix, if possible.

# Test info

- Name: history.spec.js >> Testes de Sistema E2E - Histórico de Execuções >> Deve exibir os campos de detalhe ao selecionar uma execução
- Location: tests\history.spec.js:48:7

# Error details

```
Error: expect(locator).toBeVisible() failed

Locator: locator('text=Dimensão')
Expected: visible
Error: strict mode violation: locator('text=Dimensão') resolved to 7 elements:
    1) <span class="_metricLabel_upv76_123">Dimensão</span> aka getByRole('button', { name: 'Tentativa #1 Dimensão 10x10' })
    2) <span class="_metricLabel_upv76_123">Dimensão</span> aka getByRole('button', { name: 'Tentativa #2 Dimensão 12x12' })
    3) <span class="_metricLabel_upv76_123">Dimensão</span> aka getByRole('button', { name: 'Tentativa #3 Dimensão 14x14' })
    4) <span class="_metricLabel_upv76_123">Dimensão</span> aka getByRole('button', { name: 'Tentativa #4 Dimensão 16x16' })
    5) <span class="_metricLabel_upv76_123">Dimensão</span> aka getByRole('button', { name: 'Tentativa #5 Dimensão 18x18' })
    6) <span class="_metricLabel_upv76_123">Dimensão</span> aka getByRole('button', { name: 'Tentativa #6 Dimensão 20x20' })
    7) <dt>Dimensão</dt> aka getByRole('term').filter({ hasText: 'Dimensão' })

Call log:
  - Expect "toBeVisible" with timeout 5000ms
  - waiting for locator('text=Dimensão')

```

# Page snapshot

```yaml
- generic [ref=e3]:
  - navigation [ref=e4]:
    - generic [ref=e5]: PI1 · Micromouse
    - generic [ref=e6]:
      - link "Dashboard" [ref=e7] [cursor=pointer]:
        - /url: /dashboard
      - link "Histórico" [ref=e8] [cursor=pointer]:
        - /url: /history
  - main [ref=e9]:
    - generic [ref=e10]:
      - generic [ref=e11]:
        - heading "Histórico de Execuções" [level=1] [ref=e12]
        - generic [ref=e13]: DADOS MOCKADOS
      - generic [ref=e14]:
        - generic [ref=e15]:
          - 'button "Tentativa #1 Dimensão 10x10 Tempo 00:42 Vel. média 0.48 m/s Consumo 18% SUCESSO" [active] [ref=e16] [cursor=pointer]':
            - generic [ref=e17]:
              - generic [ref=e18]: Tentativa
              - generic [ref=e19]: "#1"
            - generic [ref=e20]:
              - generic [ref=e21]:
                - generic [ref=e22]: Dimensão
                - generic [ref=e23]: 10x10
              - generic [ref=e24]:
                - generic [ref=e25]: Tempo
                - generic [ref=e26]: 00:42
              - generic [ref=e27]:
                - generic [ref=e28]: Vel. média
                - generic [ref=e29]: 0.48 m/s
              - generic [ref=e30]:
                - generic [ref=e31]: Consumo
                - generic [ref=e32]: 18%
            - generic [ref=e34]: SUCESSO
          - 'button "Tentativa #2 Dimensão 12x12 Tempo 00:31 Vel. média 0.55 m/s Consumo 14% SUCESSO" [ref=e35] [cursor=pointer]':
            - generic [ref=e36]:
              - generic [ref=e37]: Tentativa
              - generic [ref=e38]: "#2"
            - generic [ref=e39]:
              - generic [ref=e40]:
                - generic [ref=e41]: Dimensão
                - generic [ref=e42]: 12x12
              - generic [ref=e43]:
                - generic [ref=e44]: Tempo
                - generic [ref=e45]: 00:31
              - generic [ref=e46]:
                - generic [ref=e47]: Vel. média
                - generic [ref=e48]: 0.55 m/s
              - generic [ref=e49]:
                - generic [ref=e50]: Consumo
                - generic [ref=e51]: 14%
            - generic [ref=e53]: SUCESSO
          - 'button "Tentativa #3 Dimensão 14x14 Tempo — Vel. média — Consumo 5% FALHA" [ref=e54] [cursor=pointer]':
            - generic [ref=e55]:
              - generic [ref=e56]: Tentativa
              - generic [ref=e57]: "#3"
            - generic [ref=e58]:
              - generic [ref=e59]:
                - generic [ref=e60]: Dimensão
                - generic [ref=e61]: 14x14
              - generic [ref=e62]:
                - generic [ref=e63]: Tempo
                - generic [ref=e64]: —
              - generic [ref=e65]:
                - generic [ref=e66]: Vel. média
                - generic [ref=e67]: —
              - generic [ref=e68]:
                - generic [ref=e69]: Consumo
                - generic [ref=e70]: 5%
            - generic [ref=e72]: FALHA
          - 'button "Tentativa #4 Dimensão 16x16 Tempo 01:27 Vel. média 0.41 m/s Consumo 32% SUCESSO" [ref=e73] [cursor=pointer]':
            - generic [ref=e74]:
              - generic [ref=e75]: Tentativa
              - generic [ref=e76]: "#4"
            - generic [ref=e77]:
              - generic [ref=e78]:
                - generic [ref=e79]: Dimensão
                - generic [ref=e80]: 16x16
              - generic [ref=e81]:
                - generic [ref=e82]: Tempo
                - generic [ref=e83]: 01:27
              - generic [ref=e84]:
                - generic [ref=e85]: Vel. média
                - generic [ref=e86]: 0.41 m/s
              - generic [ref=e87]:
                - generic [ref=e88]: Consumo
                - generic [ref=e89]: 32%
            - generic [ref=e91]: SUCESSO
          - 'button "Tentativa #5 Dimensão 18x18 Tempo — Vel. média — Consumo 8% FALHA" [ref=e92] [cursor=pointer]':
            - generic [ref=e93]:
              - generic [ref=e94]: Tentativa
              - generic [ref=e95]: "#5"
            - generic [ref=e96]:
              - generic [ref=e97]:
                - generic [ref=e98]: Dimensão
                - generic [ref=e99]: 18x18
              - generic [ref=e100]:
                - generic [ref=e101]: Tempo
                - generic [ref=e102]: —
              - generic [ref=e103]:
                - generic [ref=e104]: Vel. média
                - generic [ref=e105]: —
              - generic [ref=e106]:
                - generic [ref=e107]: Consumo
                - generic [ref=e108]: 8%
            - generic [ref=e110]: FALHA
          - 'button "Tentativa #6 Dimensão 20x20 Tempo 02:14 Vel. média 0.38 m/s Consumo 47% SUCESSO" [ref=e111] [cursor=pointer]':
            - generic [ref=e112]:
              - generic [ref=e113]: Tentativa
              - generic [ref=e114]: "#6"
            - generic [ref=e115]:
              - generic [ref=e116]:
                - generic [ref=e117]: Dimensão
                - generic [ref=e118]: 20x20
              - generic [ref=e119]:
                - generic [ref=e120]: Tempo
                - generic [ref=e121]: 02:14
              - generic [ref=e122]:
                - generic [ref=e123]: Vel. média
                - generic [ref=e124]: 0.38 m/s
              - generic [ref=e125]:
                - generic [ref=e126]: Consumo
                - generic [ref=e127]: 47%
            - generic [ref=e129]: SUCESSO
        - complementary [ref=e130]:
          - generic [ref=e131]:
            - generic [ref=e132]:
              - 'heading "Tentativa #1" [level=2] [ref=e133]'
              - generic [ref=e134]: SUCESSO
            - generic [ref=e135]:
              - generic [ref=e136]:
                - term [ref=e137]: Dimensão
                - definition [ref=e138]: 10x10
              - generic [ref=e139]:
                - term [ref=e140]: Tempo total
                - definition [ref=e141]: 00:42
              - generic [ref=e142]:
                - term [ref=e143]: Velocidade média
                - definition [ref=e144]: 0.48 m/s
              - generic [ref=e145]:
                - term [ref=e146]: Consumo total
                - definition [ref=e147]: 18%
```

# Test source

```ts
  1  | import { test, expect } from '@playwright/test';
  2  | 
  3  | test.describe('Testes de Sistema E2E - Histórico de Execuções', () => {
  4  | 
  5  |   test('Deve exibir o título da página de histórico', async ({ page }) => {
  6  |     await page.goto('/history');
  7  | 
  8  |     await expect(page.locator('h1', { hasText: 'Histórico de Execuções' })).toBeVisible();
  9  |   });
  10 | 
  11 |   test('Deve exibir o banner de dados mockados', async ({ page }) => {
  12 |     await page.goto('/history');
  13 | 
  14 |     await expect(page.locator('text=DADOS MOCKADOS')).toBeVisible();
  15 |   });
  16 | 
  17 |   test('Deve exibir a lista de execuções históricas', async ({ page }) => {
  18 |     await page.goto('/history');
  19 | 
  20 |     // Aguardar carregamento dos dados mockados
  21 |     const execucoes = page.locator('[class*="list"] > *');
  22 |     await expect(execucoes.first()).toBeVisible({ timeout: 5000 });
  23 | 
  24 |     // Verificar que há pelo menos uma execução listada
  25 |     const count = await execucoes.count();
  26 |     expect(count).toBeGreaterThan(0);
  27 |   });
  28 | 
  29 |   test('Deve exibir painel vazio antes de selecionar uma execução', async ({ page }) => {
  30 |     await page.goto('/history');
  31 | 
  32 |     // Painel lateral deve mostrar mensagem de orientação
  33 |     await expect(page.locator('text=Selecione uma execução para ver os detalhes.')).toBeVisible();
  34 |   });
  35 | 
  36 |   test('Deve exibir detalhes ao selecionar uma execução', async ({ page }) => {
  37 |     await page.goto('/history');
  38 | 
  39 |     // Aguardar lista e clicar na primeira execução
  40 |     const primeiraExecucao = page.locator('[class*="list"] > *').first();
  41 |     await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
  42 |     await primeiraExecucao.click();
  43 | 
  44 |     // Painel de detalhes deve mostrar o número da tentativa
  45 |     await expect(page.locator('h2', { hasText: 'Tentativa #' })).toBeVisible();
  46 |   });
  47 | 
  48 |   test('Deve exibir os campos de detalhe ao selecionar uma execução', async ({ page }) => {
  49 |     await page.goto('/history');
  50 | 
  51 |     // Selecionar primeira execução
  52 |     const primeiraExecucao = page.locator('[class*="list"] > *').first();
  53 |     await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
  54 |     await primeiraExecucao.click();
  55 | 
  56 |     // Verificar campos do painel de detalhes
> 57 |     await expect(page.locator('text=Dimensão')).toBeVisible();
     |                                                 ^ Error: expect(locator).toBeVisible() failed
  58 |     await expect(page.locator('text=Tempo total')).toBeVisible();
  59 |     await expect(page.locator('text=Velocidade média')).toBeVisible();
  60 |     await expect(page.locator('text=Consumo total')).toBeVisible();
  61 |   });
  62 | 
  63 |   test('Deve exibir o status badge no painel de detalhes', async ({ page }) => {
  64 |     await page.goto('/history');
  65 | 
  66 |     const primeiraExecucao = page.locator('[class*="list"] > *').first();
  67 |     await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
  68 |     await primeiraExecucao.click();
  69 | 
  70 |     // Status badge deve estar visível no painel de detalhes
  71 |     const detailAside = page.locator('aside');
  72 |     await expect(detailAside).toBeVisible();
  73 |     const statusBadge = detailAside.locator('[class*="badge"], [class*="status"], [class*="Badge"]').first();
  74 |     await expect(statusBadge).toBeVisible();
  75 |   });
  76 | 
  77 |   test('Deve fechar o painel de detalhes ao clicar na execução selecionada novamente', async ({ page }) => {
  78 |     await page.goto('/history');
  79 | 
  80 |     const primeiraExecucao = page.locator('[class*="list"] > *').first();
  81 |     await expect(primeiraExecucao).toBeVisible({ timeout: 5000 });
  82 | 
  83 |     // Selecionar
  84 |     await primeiraExecucao.click();
  85 |     await expect(page.locator('h2', { hasText: 'Tentativa #' })).toBeVisible();
  86 | 
  87 |     // Desselecionar (toggle)
  88 |     await primeiraExecucao.click();
  89 |     await expect(page.locator('text=Selecione uma execução para ver os detalhes.')).toBeVisible();
  90 |   });
  91 | 
  92 | });
```