/**
 * NewAttempt.jsx
 *
 * tipoLabirinto é mapeado do mazeSize numérico para o formato do schema:
 *   10/12 → '4x4' | 14/16 → '8x8' | 18/20 → '16x16'
 */

import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { mazeSizeToTipoLabirinto } from '../utils/helpers';
import styles from './NewAttempt.module.css';

const MAZE_SIZES = [10, 12, 14, 16, 18, 20];

export function NewAttempt() {
  const navigate  = useNavigate();
  const [mazeSize, setMazeSize] = useState(null);

  async function handleActivate() {
    if (!mazeSize) return;
    navigate('/dashboard', { state: { mazeSize } });
  }

  return (
    <div className={styles.container}>
      <aside className={styles.sidebar}>
        <button className={styles.backBtn} onClick={() => navigate('/')}>
          ← Voltar
        </button>

        <div className={styles.metrics}>
          <MetricRow label="Dimensão"           value={mazeSize ? `${mazeSize}×${mazeSize}` : '---'} />
          <MetricRow label="Consumo de bateria" value="---" />
          <MetricRow label="Velocidade"         value="---" />
          <MetricRow label="Tempo"              value="00:00:00" />
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
                onClick={() => setMazeSize(s)}
              >
                {s}×{s}
                <span className={styles.presetSub}> ({mazeSizeToTipoLabirinto(s)})</span>
              </button>
            ))}
          </div>

          <button
            className={styles.activateBtn}
            onClick={handleActivate}
            disabled={mazeSize === null}
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
