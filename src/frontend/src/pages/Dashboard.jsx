import { useCallback, useEffect, useMemo, useState } from 'react';
import { useLocation } from 'react-router-dom';
import { MazeView } from '../components/MazeView/MazeView';
import { Sidebar } from '../components/Sidebar/Sidebar';
import { useTelemetryData } from '../hooks/useTelemetryData';
import { activateMicromouse, sendMicromouseConfiguration } from '../services/configurationService';
import styles from './Dashboard.module.css';

const DEFAULT_RUN = 1;

export function Dashboard() {
  const location = useLocation();
  const mazeSize = location.state?.mazeSize ?? 16;
  const run = location.state?.run ?? DEFAULT_RUN;
  const initiallyRunning = location.state?.activated === true;
  const [config, setConfig] = useState({
    mazeSize,
    run,
  });
  const [configStatus, setConfigStatus] = useState({
    state: 'idle',
    message: '',
  });
  const [completedFirstPassByMaze, setCompletedFirstPassByMaze] = useState({});

  const { data, running, start, reset } = useTelemetryData(config.mazeSize, config.run, initiallyRunning);

  useEffect(() => {
    if (running || data.status !== 'success' || config.run !== 1) return;

    setCompletedFirstPassByMaze((currentState) => {
      if (currentState[config.mazeSize]) return currentState;

      return {
        ...currentState,
        [config.mazeSize]: true,
      };
    });
  }, [running, data.status, config.run, config.mazeSize]);

  const hasCompletedFirstPass = Boolean(completedFirstPassByMaze[config.mazeSize]);
  const canStartSelectedRun = config.run === 1 || hasCompletedFirstPass;
  const startBlockedMessage = config.run === 2 && !hasCompletedFirstPass
    ? `Conclua a primeira passagem no labirinto ${config.mazeSize}x${config.mazeSize} para liberar a segunda.`
    : '';

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

  const handleStart = useCallback(async () => {
    if (!canStartSelectedRun) return;

    setConfigStatus({ state: 'sending', message: 'Ativando rato...' });

    try {
      const response = await activateMicromouse(config);
      start();
      setConfigStatus({
        state: 'success',
        message: response.message || 'Rato ativado com sucesso.',
      });
    } catch (error) {
      setConfigStatus({
        state: 'error',
        message: error.message || 'Falha ao ativar o rato.',
      });
    }
  }, [canStartSelectedRun, config, start]);

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
        canStartSelectedRun={canStartSelectedRun}
        startBlockedMessage={startBlockedMessage}
        onStart={handleStart}
        onReset={reset}
      />
      <div className={styles.main}>
        <div className={styles.statusBar}>
          <h1 className={styles.title}>Telemetria em Tempo Real</h1>
          <span className={styles.passLabel}>
            {config.run === 1 ? 'Primeira passagem' : 'Segunda passagem'}
          </span>
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
