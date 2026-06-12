/**
 * NewAttempt.jsx
 *
 * Ao concluir uma tentativa no Dashboard, salva na tabela HISTORICO.
 *
 * tipoLabirinto é mapeado do mazeSize numérico para o formato do schema:
 *   10/12 → '4x4' | 14/16 → '8x8' | 18/20 → '16x16'
 *
 * TODO: capturar os valores reais de corrente/tensão da telemetria
 *       ao invés de usar os defaults abaixo.
 */

import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { criarHistorico } from '../services/apiService';
import styles from './NewAttempt.module.css';

const MAZE_SIZES = [10, 12, 14, 16, 18, 20];

// Mapeia tamanho do mock para os valores aceitos pelo schema
function mazeToTipo(size) {
  if (size <= 12) return '4x4';
  if (size <= 16) return '8x8';
  return '16x16';
}

export function NewAttempt() {
  const navigate  = useNavigate();
  const [mazeSize, setMazeSize] = useState(null);
  const [saving, setSaving]     = useState(false);
  const [saveError, setSaveError] = useState(null);

  async function handleActivate() {
    if (!mazeSize) return;
    navigate('/dashboard', { state: { mazeSize } });
  }

  /**
   * Chamado pelo Dashboard quando status === 'success'.
   * Recebe os dados finais da telemetria para salvar no HISTORICO.
   *
   * @param {{ elapsedSeconds: number, batteryPercent: number, speedMps: number }} resultado
   * TODO: passar também numTentativa gerado pelo backend quando houver controle de sessão.
   */
  async function handleFinish(resultado) {
    setSaving(true);
    setSaveError(null);
    try {
      await criarHistorico({
        numTentativa:     Date.now(),          // TODO: substituir por sequência real do backend
        percentualBateria: resultado.batteryPercent,
        velocidadeMedia:   resultado.speedMps,
        tempoConclusao:    new Date().toISOString(),
        desafioCumprido:   resultado.status === 'success' ? 'SIM' : 'NAO',
        correnteEletrica:  resultado.correnteEletrica ?? 0.0,  // TODO: receber da telemetria real
        tensaoEletrica:    resultado.tensaoEletrica   ?? 0.0,  // TODO: receber da telemetria real
        tipoLabirinto:     mazeToTipo(mazeSize),
      });
    } catch (err) {
      setSaveError(err.message);
    } finally {
      setSaving(false);
    }
  }

  return (
    <div className={styles.container}>
      <aside className={styles.sidebar}>
        <button className={styles.backBtn} onClick={() => navigate('/')}>
          ← Voltar
        </button>

        <div className={styles.metrics}>
          <MetricRow label="Dimensão"           value={mazeSize ? `${mazeSize}×${mazeSize}` : '---'} />
          <MetricRow label="Consumo de bateria" value="---" />
          <MetricRow label="Velocidade"         value="---" />
          <MetricRow label="Tempo"              value="00:00:00" />
        </div>

        {saveError && <p className={styles.error}>Erro ao salvar: {saveError}</p>}
      </aside>

      <main className={styles.main}>
        <div className={styles.card}>
          <p className={styles.cardLabel}>Escolha o tamanho do labirinto:</p>

          <div className={styles.presets}>
            {MAZE_SIZES.map((s) => (
              <button
                key={s}
                className={`${styles.presetBtn} ${mazeSize === s ? styles.presetActive : ''}`}
                onClick={() => setMazeSize(s)}
              >
                {s}×{s}
                <span className={styles.presetSub}> ({mazeToTipo(s)})</span>
              </button>
            ))}
          </div>

          <button
            className={styles.activateBtn}
            onClick={handleActivate}
            disabled={mazeSize === null || saving}
          >
            {saving ? 'Salvando...' : 'Ativar rato'}
          </button>
        </div>
      </main>
    </div>
  );
}

function MetricRow({ label, value }) {
  return (
    <div className={styles.metricRow}>
      <span className={styles.metricLabel}>{label}</span>
      <div className={styles.metricDivider} />
      <span className={styles.metricValue}>{value}</span>
    </div>
  );
}
