import { useCallback, useMemo, useState } from 'react';
import { useLocation } from 'react-router-dom';
import { MazeView } from '../components/MazeView/MazeView';
import { Sidebar } from '../components/Sidebar/Sidebar';
import { useTelemetryData } from '../hooks/useTelemetryData';
import { sendMicromouseConfiguration } from '../services/configurationService';
import styles from './Dashboard.module.css';

const DEFAULT_RUN = 1;

export function Dashboard() {
  const location = useLocation();
  const mazeSize = location.state?.mazeSize ?? 10;
  const [config, setConfig] = useState({
    mazeSize,
    run: DEFAULT_RUN,
  });
  const [configStatus, setConfigStatus] = useState({
    state: 'idle',
    message: '',
  });

  const { data, running, start, reset } = useTelemetryData(config.mazeSize);

  const handleConfigChange = useCallback((partialConfig) => {
    setConfig((currentConfig) => ({
      ...currentConfig,
      ...partialConfig,
    }));
    setConfigStatus({ state: 'idle', message: '' });
  }, []);

  const handleSendConfiguration = useCallback(async () => {
    setConfigStatus({ state: 'sending', message: 'Enviando configurações...' });

    try {
      const response = await sendMicromouseConfiguration(config);
      setConfigStatus({
        state: 'success',
        message: response.message || 'Configurações enviadas com sucesso.',
      });
    } catch (error) {
      setConfigStatus({
        state: 'error',
        message: error.message || 'Falha ao enviar configurações.',
      });
    }
  }, [config]);

  const sidebarConfig = useMemo(() => ({
    mazeSize: config.mazeSize,
    run: config.run,
  }), [config.mazeSize, config.run]);

  return (
    <div className={styles.layout}>
      <Sidebar
        data={data}
        running={running}
        config={sidebarConfig}
        onConfigChange={handleConfigChange}
        onSendConfiguration={handleSendConfiguration}
        configStatus={configStatus}
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
