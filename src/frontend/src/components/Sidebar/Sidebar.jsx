import { useNavigate } from 'react-router-dom';
import { Button } from '../Button/Button';
import { StatusBadge } from '../StatusBadge/StatusBadge';
import { formatBattery, formatMazeDimension, formatSpeed, formatTime } from '../../utils/helpers';
import styles from './Sidebar.module.css';

function InfoRow({ label, value }) {
  return (
    <div className={styles.infoRow}>
      <span className={styles.label}>{label}</span>
      <span className={styles.value}>{value}</span>
    </div>
  );
}

export function Sidebar({ data, running, onStart, onReset }) {
  const navigate = useNavigate();

  return (
    <aside className={styles.sidebar}>
      <div className={styles.header}>
        <span className={styles.logo}>Micromouse</span>
        <span className="mock-banner">MOCK</span>
      </div>

      <section className={styles.section}>
        <h3 className={styles.sectionTitle}>Telemetria</h3>
        <InfoRow label="Dimensão" value={formatMazeDimension(data.mazeSize)} />
        <InfoRow label="Bateria" value={formatBattery(data.batteryPercent)} />
        <InfoRow label="Velocidade" value={formatSpeed(data.speedMps)} />
        <InfoRow label="Tempo" value={formatTime(data.elapsedSeconds)} />
      </section>

      <section className={styles.section}>
        <h3 className={styles.sectionTitle}>Status</h3>
        <StatusBadge status={data.status} />
      </section>

      <section className={styles.controls}>
        <Button onClick={onStart} disabled={running || data.status === 'success'} fullWidth>
          {data.status === 'success' ? 'Concluído' : running ? 'Executando...' : 'Ativar rato'}
        </Button>
        <Button onClick={onReset} variant="ghost" fullWidth>
          Reiniciar
        </Button>
        <Button onClick={() => navigate('/new-attempt')} variant="ghost" fullWidth>
          Novo labirinto
        </Button>
      </section>
    </aside>
  );
}