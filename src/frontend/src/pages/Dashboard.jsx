import { MazeView } from '../components/MazeView/MazeView';
import { Sidebar } from '../components/Sidebar/Sidebar';
import { useTelemetryData } from '../hooks/useTelemetryData';
import styles from './Dashboard.module.css';

export function Dashboard() {
  const { data, connected, start, reset } = useTelemetryData();

  return (
    <div className={styles.layout}>
      <Sidebar
        data={data}
        connected={connected}
        onStart={start}
        onReset={reset}
      />
      <div className={styles.main}>
        <div className={styles.statusBar}>
          <h1 className={styles.title}>Telemetria em Tempo Real</h1>
          {!connected && (
            <span className={styles.warnMsg}>Conectando ao broker...</span>
          )}
          {connected && data.status === 'waiting' && (
            <span className={styles.waitingMsg}>Aguardando comando de início...</span>
          )}
          {connected && data.status === 'running' && (
            <span className={styles.livePulse}>● AO VIVO</span>
          )}
          {connected && data.status === 'stuck' && (
            <span className={styles.warnMsg}>⚠ Rato travado</span>
          )}
          {connected && data.status === 'success' && (
            <span className={styles.successMsg}>✓ Labirinto concluído com sucesso!</span>
          )}
        </div>
        {data.grid ? (
          <MazeView
            grid={data.grid}
            position={data.position}
            goal={data.goal}
            start={data.start}
            visitedPath={data.visitedPath}
            status={data.status}
          />
        ) : (
          <p className={styles.waiting}>Aguardando dados da ESP32...</p>
        )}
      </div>
    </div>
  );
}
