import { Button } from '../Button/Button';
import { StatusBadge } from '../StatusBadge/StatusBadge';
import {formatBattery, formatMazeDimension, formatSpeed, formatTime} from '../../utils/helpers';
import styles from './Sidebar.module.css';

function InfoRow({ label, value }) {
  return (
    <div className={styles.infoRow}>
      <span className={styles.label}>{label}</span>
      <span className={styles.value}>{value}</span>
    </div>
  );
}

export function Sidebar({ data, connected, onStart, onReset }) {
  return (
    <aside className={styles.sidebar}>
      <div className={styles.header}>
        <span className={styles.logo}>Micromouse</span>
        <span className={`${styles.connectionBadge} ${connected ? styles.connectionOnline : styles.connectionOffline}`}>
          {connected ? 'CONECTADO' : 'OFFLINE'}
        </span>
      </div>

      <section className={styles.section}>
        <h3 className={styles.sectionTitle}>Telemetria</h3>
        <InfoRow label="Dimensão" value={data.mazeSize ? formatMazeDimension(data.mazeSize) : '---'} />
        <InfoRow label="Bateria" value={data.batteryPercent != null ? formatBattery(data.batteryPercent) : '---'} />
        <InfoRow label="Velocidade" value={data.speedMps != null ? formatSpeed(data.speedMps) : '---'} />
        <InfoRow label="Tempo" value={formatTime(data.elapsedSeconds ?? 0)} />
      </section>

      <section className={styles.section}>
        <h3 className={styles.sectionTitle}>Status</h3>
        <StatusBadge status={data.status} />
      </section>

      <section className={styles.controls}>
        <Button onClick={onStart} disabled={!connected || data.status === 'success'} fullWidth>
          Iniciar corrida
        </Button>
        <Button onClick={onReset} variant="ghost" fullWidth>
          Limpar tela
        </Button>
      </section>
    </aside>
  );
}
