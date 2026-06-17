import { Button } from '../Button/Button';
import { StatusBadge } from '../StatusBadge/StatusBadge';
import { formatBattery, formatMazeDimension, formatSpeed, formatTime } from '../../utils/helpers';
import styles from './Sidebar.module.css';

const MAZE_SIZE_OPTIONS = [4, 8, 16];
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

function OptionButton({ active, onClick, children, ariaLabel }) {
  return (
    <button
      type="button"
      className={`${styles.optionButton} ${active ? styles.optionButtonActive : ''}`}
      onClick={onClick}
      aria-pressed={active}
      aria-label={ariaLabel}
    >
      {children}
    </button>
  );
}

export function Sidebar({
  data,
  running,
  config,
  onConfigChange,
  onSendConfiguration,
  configStatus,
  canStartSelectedRun,
  startBlockedMessage,
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
          <div className={styles.field}>
            <span className={styles.fieldLabel}>Tamanho do labirinto</span>
            <div className={styles.optionGroup}>
              {MAZE_SIZE_OPTIONS.map((mazeSize) => (
                <OptionButton
                  key={mazeSize}
                  active={config.mazeSize === mazeSize}
                  onClick={() => onConfigChange({ mazeSize })}
                  ariaLabel={`Selecionar labirinto de ${mazeSize} por ${mazeSize}`}
                >
                  {mazeSize}x{mazeSize}
                </OptionButton>
              ))}
            </div>
          </div>

          <div className={styles.field}>
            <span className={styles.fieldLabel}>Execução</span>
            <div className={styles.optionGroup}>
              {RUN_OPTIONS.map((option) => (
                <OptionButton
                  key={option.value}
                  active={config.run === option.value}
                  onClick={() => onConfigChange({ run: option.value })}
                  ariaLabel={`Selecionar ${option.label.toLowerCase()}`}
                >
                  <span>{option.label}</span>
                </OptionButton>
              ))}
            </div>
            <p className={styles.helperText}>
              Na primeira execução, o rato identifica o caminho. Na segunda, ele usa o caminho descoberto para ir mais rápido.
            </p>
          </div>
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
        <Button
          onClick={onStart}
          disabled={running || data.status === 'success' || !canStartSelectedRun || configStatus.state === 'sending'}
          fullWidth
        >
          {data.status === 'success' ? 'Concluído' : running ? 'Executando...' : 'Ativar rato'}
        </Button>
        {startBlockedMessage && (
          <p className={styles.startHint} role="status" aria-live="polite">
            {startBlockedMessage}
          </p>
        )}
        <Button onClick={onReset} variant="ghost" fullWidth>
          Reiniciar
        </Button>
      </section>
    </aside>
  );
}
