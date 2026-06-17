import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { activateMicromouse, sendMicromouseConfiguration } from '../services/configurationService';
import styles from './NewAttempt.module.css';

const MAZE_SIZES = [4, 8, 16];
const RUN_OPTIONS = [
  { value: 1, label: '1ª Passagem' },
  { value: 2, label: '2ª Passagem' },
];

export function NewAttempt() {
  const navigate = useNavigate();
  const [mazeSize, setMazeSize] = useState(null);
  const [run, setRun] = useState(1);
  const [configStatus, setConfigStatus] = useState({
    state: 'idle',
    message: '',
  });
  const [activationStatus, setActivationStatus] = useState({
    state: 'idle',
    message: '',
  });

  async function handleSendConfiguration() {
    if (mazeSize === null) return;
    setConfigStatus({ state: 'sending', message: 'Enviando configurações...' });

    try {
      const response = await sendMicromouseConfiguration({ mazeSize, run });
      setConfigStatus({
        state: 'success',
        message: response.message || 'Configurações enviadas com sucesso.',
      });
    } catch (error) {
      setConfigStatus({
        state: 'error',
        message: error.message || 'Falha ao enviar configurações.',
      });
    }
  }

  async function handleActivate() {
    if (mazeSize === null) return;
    setActivationStatus({ state: 'sending', message: 'Ativando rato...' });

    try {
      await activateMicromouse({ mazeSize, run });
      navigate('/dashboard', { state: { mazeSize, run, activated: true } });
    } catch (error) {
      setActivationStatus({
        state: 'error',
        message: error.message || 'Falha ao ativar o rato.',
      });
    }
  }

  return (
    <div className={styles.container}>
      <aside className={styles.sidebar}>
        <button className={styles.backBtn} onClick={() => navigate('/')}>
          ← Voltar
        </button>

        <div className={styles.metrics}>
          <MetricRow label="Dimensão" value={mazeSize ? `${mazeSize}x${mazeSize}` : '---'} />
          <MetricRow label="Consumo de bateria" value="---" />
          <MetricRow label="Velocidade" value="---" />
          <MetricRow label="Tempo" value="00:00:00" />
        </div>
      </aside>

      <main className={styles.main}>
        <div className={styles.card}>
          <p className={styles.cardLabel}>Escolha o tamanho do labirinto:</p>

          <div className={styles.presets}>
            {MAZE_SIZES.map((s) => (
              <button
                key={s}
                className={`${styles.presetBtn} ${mazeSize === s ? styles.presetActive : ''}`}
                onClick={() => {
                  setMazeSize(s);
                  setConfigStatus({ state: 'idle', message: '' });
                  setActivationStatus({ state: 'idle', message: '' });
                }}
              >
                {s}×{s}
              </button>
            ))}
          </div>

          <p className={styles.cardLabel}>Escolha a execução:</p>

          <div className={styles.runOptions}>
            {RUN_OPTIONS.map((option) => (
              <button
                key={option.value}
                type="button"
                className={`${styles.runBtn} ${run === option.value ? styles.runActive : ''}`}
                onClick={() => {
                  setRun(option.value);
                  setConfigStatus({ state: 'idle', message: '' });
                  setActivationStatus({ state: 'idle', message: '' });
                }}
              >
                {option.label}
              </button>
            ))}
          </div>

          <div className={styles.buttonGroup}>
            <button
              className={styles.sendBtn}
              onClick={handleSendConfiguration}
              disabled={mazeSize === null || configStatus.state === 'sending'}
            >
              {configStatus.state === 'sending' ? 'Enviando...' : 'Enviar para o Rato'}
            </button>

            <button
              className={styles.activateBtn}
              onClick={handleActivate}
              disabled={mazeSize === null || activationStatus.state === 'sending'}
            >
              {activationStatus.state === 'sending' ? 'Ativando...' : 'Ativar Rato'}
            </button>
          </div>

          {configStatus.message && (
            <p className={`${styles.feedback} ${styles[configStatus.state]}`} role="status" aria-live="polite">
              {configStatus.message}
            </p>
          )}
          {activationStatus.message && (
            <p className={`${styles.feedback} ${styles[activationStatus.state]}`} role="status" aria-live="polite">
              {activationStatus.message}
            </p>
          )}
        </div>
      </main>
    </div>
  );
}

function MetricRow({ label, value }) {
  return (
    <div className={styles.metricRow}>
      <span className={styles.metricLabel}>{label}</span>
      <div className={styles.metricDivider} />
      <span className={styles.metricValue}>{value}</span>
    </div>
  );
}

