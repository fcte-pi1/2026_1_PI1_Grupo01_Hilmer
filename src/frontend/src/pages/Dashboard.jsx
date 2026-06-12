import { useEffect, useRef, useState } from 'react';
import { useLocation } from 'react-router-dom';
import { MazeView } from '../components/MazeView/MazeView';
import { Sidebar } from '../components/Sidebar/Sidebar';
import { useTelemetryData } from '../hooks/useTelemetryData';
import { criarHistorico } from '../services/apiService';
import { mazeSizeToTipoLabirinto } from '../utils/helpers';
import styles from './Dashboard.module.css';

export function Dashboard() {
  const location = useLocation();
  const mazeSize = location.state?.mazeSize ?? 10;
  const { data, running, start, reset } = useTelemetryData(mazeSize);
  const [saveError, setSaveError] = useState(null);
  const hasSavedRef = useRef(false);

  useEffect(() => {
    if (data.status !== 'success' || hasSavedRef.current) return;

    hasSavedRef.current = true;

    async function persistAttempt() {
      try {
        await criarHistorico({
          numTentativa: Date.now(), // TODO: substituir por sequência real do backend
          percentualBateria: data.batteryPercent,
          velocidadeMedia: data.speedMps,
          tempoConclusao: new Date().toISOString(),
          desafioCumprido: 'SIM',
          correnteEletrica: data.correnteEletrica ?? 0.0, // TODO: receber da telemetria real
          tensaoEletrica: data.tensaoEletrica ?? 0.0, // TODO: receber da telemetria real
          tipoLabirinto: mazeSizeToTipoLabirinto(mazeSize),
        });
        setSaveError(null);
      } catch (err) {
        setSaveError(err.message);
        hasSavedRef.current = false;
      }
    }

    persistAttempt();
  }, [data, mazeSize]);

  function handleStart() {
    setSaveError(null);
    hasSavedRef.current = false;
    start();
  }

  function handleReset() {
    setSaveError(null);
    hasSavedRef.current = false;
    reset();
  }

  return (
    <div className={styles.layout}>
      <Sidebar
        data={data}
        running={running}
        onStart={handleStart}
        onReset={handleReset}
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
        {saveError && <p className={styles.successMsg}>Erro ao salvar histórico: {saveError}</p>}
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
