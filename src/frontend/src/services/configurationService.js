const CONFIGURATION_ENDPOINT = '/api/micromouse/configuracoes';
const ACTIVATION_ENDPOINT = '/api/micromouse/ativar';

function buildMicromousePayload({ mazeSize, run }) {
  return {
    mazeSize,
    run,
    tipoLabirinto: `${mazeSize}x${mazeSize}`,
    execucao: run === 1 ? 'primeira' : 'segunda',
  };
}

async function postMicromouseCommand(endpoint, config, fallbackMessage) {
  const payload = buildMicromousePayload(config);

  const response = await fetch(endpoint, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
    },
    body: JSON.stringify(payload),
  });

  const data = await response.json().catch(() => ({}));

  if (!response.ok) {
    throw new Error(data.error || fallbackMessage);
  }

  return data;
}

export async function sendMicromouseConfiguration(config) {
  return postMicromouseCommand(
    CONFIGURATION_ENDPOINT,
    config,
    'Não foi possível enviar as configurações.',
  );
}

export async function activateMicromouse(config) {
  return postMicromouseCommand(
    ACTIVATION_ENDPOINT,
    config,
    'Não foi possível ativar o rato.',
  );
}

export { buildMicromousePayload };
