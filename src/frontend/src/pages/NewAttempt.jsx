import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import styles from './NewAttempt.module.css';

const PRESET_SIZES = [8, 10, 16];

export function NewAttempt() {
  const navigate = useNavigate();
  const [mazeSize, setMazeSize] = useState('');
  const [error, setError] = useState('');

  function handleSizeChange(e) {
    const val = e.target.value.replace(/\D/g, '');
    setMazeSize(val);
    setError('');
  }

  function handlePreset(size) {
    setMazeSize(String(size));
    setError('');
  }

  function handleActivate() {
    const size = parseInt(mazeSize, 10);
    if (!mazeSize || isNaN(size) || size < 4 || size > 32) {
      setError('Insira um tamanho entre 4 e 32.');
      return;
    }
    // Passa o tamanho via state de navegação para o Dashboard usar futuramente
    navigate('/dashboard', { state: { mazeSize: size } });
  }

  return (
    <div className={styles.container}>
      <aside className={styles.sidebar}>
        <button className={styles.backBtn} onClick={() => navigate('/')}>
          ← Voltar
        </button>

        <div className={styles.metrics}>
          <MetricRow label="Dimensão" value="---" />
          <MetricRow label="Consumo de bateria" value="---" />
          <MetricRow label="Velocidade" value="---" />
          <MetricRow label="Tempo" value="00:00:00" />
        </div>
      </aside>

      <main className={styles.main}>
        <div className={styles.card}>
          <p className={styles.cardLabel}>Insira o tamanho do labirinto:</p>

          <div className={styles.presets}>
            {PRESET_SIZES.map((s) => (
              <button
                key={s}
                className={`${styles.presetBtn} ${mazeSize === String(s) ? styles.presetActive : ''}`}
                onClick={() => handlePreset(s)}
              >
                {s}×{s}
              </button>
            ))}
          </div>

          <input
            className={`${styles.input} ${error ? styles.inputError : ''}`}
            type="text"
            inputMode="numeric"
            maxLength={2}
            placeholder="ex: 16"
            value={mazeSize}
            onChange={handleSizeChange}
          />

          {error && <p className={styles.error}>{error}</p>}

          <button
            className={styles.activateBtn}
            onClick={handleActivate}
            disabled={!mazeSize}
          >
            Ativar rato
          </button>
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
