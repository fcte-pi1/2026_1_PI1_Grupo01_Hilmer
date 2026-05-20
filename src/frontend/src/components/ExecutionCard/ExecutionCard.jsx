import { StatusBadge } from '../StatusBadge/StatusBadge';
import {
  formatBattery,
  formatMazeDimension,
  formatSpeed,
  formatTime,
} from '../../utils/helpers';
import styles from './ExecutionCard.module.css';

export function ExecutionCard({ execution, selected, onClick }) {
  const { attempt, mazeSize, totalTimeSeconds, avgSpeedMps, totalBatteryUsed, status } = execution;

  return (
    <article
      className={`${styles.card} ${selected ? styles.selected : ''}`}
      onClick={onClick}
      role="button"
      tabIndex={0}
      onKeyDown={(e) => e.key === 'Enter' && onClick?.()}
    >
      <div className={styles.attempt}>
        <span className={styles.attemptLabel}>Tentativa</span>
        <span className={styles.attemptNumber}>#{attempt}</span>
      </div>

      <div className={styles.metrics}>
        <div className={styles.metric}>
          <span className={styles.metricLabel}>Dimensão</span>
          <span className={styles.metricValue}>{formatMazeDimension(mazeSize)}</span>
        </div>
        <div className={styles.metric}>
          <span className={styles.metricLabel}>Tempo</span>
          <span className={styles.metricValue}>
            {status === 'failure' ? '—' : formatTime(totalTimeSeconds)}
          </span>
        </div>
        <div className={styles.metric}>
          <span className={styles.metricLabel}>Vel. média</span>
          <span className={styles.metricValue}>
            {status === 'failure' ? '—' : formatSpeed(avgSpeedMps)}
          </span>
        </div>
        <div className={styles.metric}>
          <span className={styles.metricLabel}>Consumo</span>
          <span className={styles.metricValue}>{formatBattery(totalBatteryUsed)}</span>
        </div>
      </div>

      <div className={styles.statusCol}>
        <StatusBadge status={status} />
      </div>
    </article>
  );
}
