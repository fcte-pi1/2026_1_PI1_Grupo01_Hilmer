import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { sendMicromouseConfiguration } from '../services/configurationService';
import styles from './NewAttempt.module.css';

const MAZE_SIZES = [10, 12, 14, 16, 18, 20];
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

  function handleActivate() {
    navigate('/dashboard', { state: { mazeSize, run } });
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
              disabled={mazeSize === null}
            >
              Ativar Rato
            </button>
          </div>

          {configStatus.message && (
            <p className={`${styles.feedback} ${styles[configStatus.state]}`} role="status" aria-live="polite">
              {configStatus.message}
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

