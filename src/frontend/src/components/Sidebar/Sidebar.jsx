import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { Button } from '../Button/Button';
import { StatusBadge } from '../StatusBadge/StatusBadge';
import { formatBattery, formatMazeDimension, formatSpeed, formatTime } from '../../utils/helpers';
import { formatStartCorner } from '../../utils/startCorner';
import styles from './Sidebar.module.css';

function InfoRow({ label, value }) {
  return (
    <div className={styles.infoRow}>
      <span className={styles.label}>{label}</span>
      <span className={styles.value}>{value}</span>
    </div>
  );
}

function ConnectionBadge({ connected, onlineLabel = 'CONECTADO' }) {
  return (
    <span className={`${styles.connectionBadge} ${connected ? styles.connectionOnline : styles.connectionOffline}`}>
      {connected ? onlineLabel : 'OFFLINE'}
    </span>
  );
}

export function Sidebar({
  data,
  connected,
  esp32Connected,
  onStart,
  onStop,
  onReset,
  onReconnectEsp32,
  selectedStartCorner,
}) {
  const navigate = useNavigate();
  const [reconnecting, setReconnecting] = useState(false);

  async function handleReconnectEsp32() {
    setReconnecting(true);
    try {
      await onReconnectEsp32?.();
    } finally {
      setReconnecting(false);
    }
  }

  return (
    <aside className={styles.sidebar}>
      <div className={styles.header}>
        <span className={styles.logo}>Micromouse</span>
        <ConnectionBadge connected={connected} />
      </div>

      <section className={styles.section}>
        <h3 className={styles.sectionTitle}>Telemetria</h3>
        <InfoRow label="ESP32" value={<ConnectionBadge connected={esp32Connected} onlineLabel="CONECTADA" />} />
        <InfoRow label="Dimensão" value={data.mazeSize ? formatMazeDimension(data.mazeSize) : '---'} />
        <InfoRow label="Canto inicial" value={formatStartCorner(data.startCorner ?? selectedStartCorner)} />
        <InfoRow label="Tentativa nº" value={data.attemptNumber ?? '---'} />
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
        <Button onClick={onStop} variant="ghost" disabled={!connected || !esp32Connected} fullWidth>
          Parar robô
        </Button>
        <Button
          onClick={handleReconnectEsp32}
          variant="ghost"
          disabled={reconnecting || esp32Connected}
          fullWidth
        >
          {reconnecting ? 'Reconectando...' : esp32Connected ? 'ESP32 conectada' : 'Reconectar ESP32'}
        </Button>
        <Button onClick={onReset} variant="ghost" fullWidth>
          Limpar tela
        </Button>
        <Button onClick={() => navigate('/new-attempt')} variant="ghost" fullWidth>
          Novo labirinto
        </Button>
      </section>
    </aside>
  );
}
