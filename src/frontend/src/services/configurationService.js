const CONFIGURATION_ENDPOINT = '/api/micromouse/configuracoes';

export async function sendMicromouseConfiguration({ mazeSize, run }) {
  const payload = {
    mazeSize,
    run,
    tipoLabirinto: `${mazeSize}x${mazeSize}`,
    execucao: run === 1 ? 'primeira' : 'segunda',
  };

  const response = await fetch(CONFIGURATION_ENDPOINT, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(payload),
  });

  const data = await response.json().catch(() => ({}));

  if (!response.ok) {
    throw new Error(data.error || 'Não foi possível enviar as configurações.');
  }

  return data;
}