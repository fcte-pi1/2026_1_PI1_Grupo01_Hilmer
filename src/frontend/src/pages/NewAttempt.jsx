/**
 * NewAttempt.jsx
 *
 * tipoLabirinto corresponde diretamente ao schema: '4x4' | '8x8' | '16x16'
 */

import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import {
  DEFAULT_START_CORNER,
  formatStartCorner,
  START_CORNERS,
} from '../utils/startCorner';
import styles from './NewAttempt.module.css';

const MAZE_OPTIONS = [
  { size: 4,  label: '4×4',   tipo: '4x4'   },
  { size: 8,  label: '8×8',   tipo: '8x8'   },
  { size: 16, label: '16×16', tipo: '16x16' },
];

export function NewAttempt() {
  const navigate  = useNavigate();
  const [selected, setSelected] = useState(null); // { size, label, tipo }
  const [startCorner, setStartCorner] = useState(DEFAULT_START_CORNER);

  async function handleActivate() {
    if (!selected) return;
    navigate('/dashboard', { state: { mazeSize: selected.size, startCorner } });
  }

  return (
    <div className={styles.container}>
      <aside className={styles.sidebar}>
        <button className={styles.backBtn} onClick={() => navigate('/')}>
          ← Voltar
        </button>

        <div className={styles.metrics}>
          <MetricRow label="Dimensão"           value={selected ? selected.label : '---'} />
          <MetricRow label="Canto inicial"      value={formatStartCorner(startCorner)} />
          <MetricRow label="Consumo de bateria" value="---" />
          <MetricRow label="Velocidade"         value="---" />
          <MetricRow label="Tempo"              value="00:00:00" />
        </div>
      </aside>

      <main className={styles.main}>
        <div className={styles.card}>
          <p className={styles.cardLabel}>Escolha o tamanho do labirinto:</p>

          <div className={styles.presets}>
            {MAZE_OPTIONS.map((opt) => (
              <button
                key={opt.size}
                className={`${styles.presetBtn} ${selected?.size === opt.size ? styles.presetActive : ''}`}
                onClick={() => setSelected(opt)}
              >
                {opt.label}
                <span className={styles.presetSub}> ({opt.tipo})</span>
              </button>
            ))}
          </div>

          <p className={styles.cardLabel}>Escolha o canto inicial:</p>

          <div className={`${styles.presets} ${styles.cornerGrid}`}>
            {START_CORNERS.map((corner) => (
              <button
                key={corner.value}
                className={`${styles.presetBtn} ${startCorner === corner.value ? styles.presetActive : ''}`}
                onClick={() => setStartCorner(corner.value)}
              >
                {corner.label}
              </button>
            ))}
          </div>

          <button
            className={styles.activateBtn}
            onClick={handleActivate}
            disabled={selected === null}
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
