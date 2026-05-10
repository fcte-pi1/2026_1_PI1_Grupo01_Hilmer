import { useLocation } from 'react-router-dom';
import { MazeView } from '../components/MazeView/MazeView';
import { Sidebar } from '../components/Sidebar/Sidebar';
import { useTelemetryData } from '../hooks/useTelemetryData';
import styles from './Dashboard.module.css';

export function Dashboard() {
  const location = useLocation();
  const mazeSize = location.state?.mazeSize ?? 10;
  const { data, running, start, reset } = useTelemetryData(mazeSize);

  return (
    <div className={styles.layout}>
      <Sidebar
        data={data}
        running={running}
        onStart={start}
        onReset={reset}
      />
      <div className={styles.main}>
        <div className={styles.statusBar}>
          <h1 className={styles.title}>Telemetria em Tempo Real</h1>
          {data.status === 'running' && (
            <span className={styles.livePulse}>● AO VIVO</span>
          )}
          {data.status === 'success' && (
            <span className={styles.successMsg}>✓ Labirinto concluído com sucesso!</span>
          )}
        </div>
        <MazeView
          grid={data.grid}
          position={data.position}
          goal={data.goal}
          start={data.start}
          visitedPath={data.visitedPath}
          status={data.status}
        />
      </div>
    </div>
  );
}
