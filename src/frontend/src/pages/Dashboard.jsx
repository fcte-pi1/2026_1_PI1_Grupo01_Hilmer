import { useEffect, useRef, useState } from 'react';
import { useLocation } from 'react-router-dom';
import { MazeView } from '../components/MazeView/MazeView';
import { Sidebar } from '../components/Sidebar/Sidebar';
import { useTelemetryData } from '../hooks/useTelemetryData';
import { criarHistorico, criarPassoTrajeto } from '../services/apiService';
import { mazeSizeToTipoLabirinto } from '../utils/helpers';
import styles from './Dashboard.module.css';

function inferDirection(previousPosition, currentPosition, nextPosition) {
  const origin = previousPosition ?? currentPosition;
  const destination = nextPosition ?? currentPosition;
  const rowDelta = destination[0] - origin[0];
  const colDelta = destination[1] - origin[1];

  if (rowDelta < 0) return 'NORTE';
  if (rowDelta > 0) return 'SUL';
  if (colDelta > 0) return 'LESTE';
  if (colDelta < 0) return 'OESTE';
  return 'NORTE';
}

function buildTrajectoryPayload(numTentativa, visitedPath = []) {
  return visitedPath.map((position, index) => ({
    numTentativa,
    passo: index + 1,
    pos_h: position[1],
    pos_v: position[0],
    direcao: inferDirection(
      index > 0 ? visitedPath[index - 1] : null,
      position,
      visitedPath[index + 1] ?? null,
    ),
  }));
}

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
        const historicoResponse = await criarHistorico({
          percentualBateria: data.batteryPercent,
          velocidadeMedia: data.speedMps,
          tempoConclusao: new Date().toISOString(),
          desafioCumprido: 'SIM',
          correnteEletrica: data.correnteEletrica ?? 0.0, // TODO: receber da telemetria real
          tensaoEletrica: data.tensaoEletrica ?? 0.0, // TODO: receber da telemetria real
          tipoLabirinto: mazeSizeToTipoLabirinto(mazeSize),
        });

        const numTentativa = historicoResponse?.data?.numtentativa;
        const trajectoryPayload = buildTrajectoryPayload(numTentativa, data.visitedPath);

        if (numTentativa && trajectoryPayload.length > 0) {
          await Promise.all(trajectoryPayload.map((step) => criarPassoTrajeto(step)));
        }

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
