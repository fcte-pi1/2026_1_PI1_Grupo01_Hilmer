const DEFAULT_TIMEOUT_MS = 15_000;

export async function isApiHealthy(baseUrl) {
  try {
    const response = await fetch(`${baseUrl}/api/health`);
    return response.ok;
  } catch {
    return false;
  }
}

export async function waitForApi(baseUrl, timeoutMs = DEFAULT_TIMEOUT_MS) {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    if (await isApiHealthy(baseUrl)) {
      return;
    }
    await new Promise((resolve) => setTimeout(resolve, 200));
  }

  throw new Error(
    `API indisponível em ${baseUrl}. Inicie o backend (npm run dev em src/backend) e o Postgres antes de rodar test:integration.`,
  );
}
