import { Button } from '../Button/Button';
import { StatusBadge } from '../StatusBadge/StatusBadge';
import {formatBattery, formatMazeDimension, formatSpeed, formatTime} from '../../utils/helpers';
import styles from './Sidebar.module.css';

const MAZE_SIZE_OPTIONS = [10, 12, 14, 16, 18, 20];
const RUN_OPTIONS = [
  { value: 1, label: 'Primeira passagem' },
  { value: 2, label: 'Segunda passagem' },
];

function InfoRow({ label, value }) {
  return (
    <div className={styles.infoRow}>
      <span className={styles.label}>{label}</span>
      <span className={styles.value}>{value}</span>
    </div>
  );
}

function SelectField({ label, children }) {
  return (
    <label className={styles.field}>
      <span className={styles.fieldLabel}>{label}</span>
      {children}
    </label>
  );
}

export function Sidebar({
  data,
  running,
  config,
  onConfigChange,
  onSendConfiguration,
  configStatus,
  onStart,
  onReset,
}) {
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

      <section className={styles.section}>
        <h3 className={styles.sectionTitle}>Configurações</h3>

        <div className={styles.configGrid}>
          <SelectField label="Tamanho do labirinto">
            <select
              className={styles.select}
              value={config.mazeSize}
              onChange={(event) => onConfigChange({ mazeSize: Number(event.target.value) })}
            >
              {MAZE_SIZE_OPTIONS.map((mazeSize) => (
                <option key={mazeSize} value={mazeSize}>
                  {mazeSize}x{mazeSize}
                </option>
              ))}
            </select>
          </SelectField>

          <SelectField label="Execução">
            <select
              className={styles.select}
              value={config.run}
              onChange={(event) => onConfigChange({ run: Number(event.target.value) })}
            >
              {RUN_OPTIONS.map((option) => (
                <option key={option.value} value={option.value}>
                  {option.label}
                </option>
              ))}
            </select>
          </SelectField>
        </div>

        <Button onClick={onSendConfiguration} variant="ghost" fullWidth disabled={configStatus.state === 'sending'}>
          {configStatus.state === 'sending' ? 'Enviando...' : 'Enviar configurações'}
        </Button>

        {configStatus.message && (
          <p className={`${styles.feedback} ${styles[configStatus.state]}`} role="status" aria-live="polite">
            {configStatus.message}
          </p>
        )}
      </section>

      <section className={styles.controls}>
        <Button onClick={onStart} disabled={running || data.status === 'success'} fullWidth>
          {data.status === 'success' ? 'Concluído' : running ? 'Executando...' : 'Ativar rato'}
        </Button>
        <Button onClick={onReset} variant="ghost" fullWidth>
          Reiniciar
        </Button>
      </section>
    </aside>
  );
}
