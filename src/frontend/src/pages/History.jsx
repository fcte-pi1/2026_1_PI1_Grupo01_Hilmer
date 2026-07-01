/**
 * History.jsx
 *
 * Lista o histórico de tentativas da tabela HISTORICO.
 */

import { useEffect, useState } from 'react';
import { MazeView } from '../components/MazeView/MazeView';
import { analisarTentativa, listarHistorico } from '../services/apiService';
import { analysisToMazeViewProps } from '../services/telemetryService';
import styles from './History.module.css';

const TIPO_LABEL = { '4x4': '4×4', '8x8': '8×8', '16x16': '16×16' };
const FILTER_OPTIONS = [
  { value: 'ALL', label: 'Todos' },
  { value: '4x4', label: '4×4' },
  { value: '8x8', label: '8×8' },
  { value: '16x16', label: '16×16' },
];

export function History() {
  const [historico, setHistorico] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState(null);
  const [sizeFilter, setSizeFilter] = useState('ALL');
  const [selectedAttempt, setSelectedAttempt] = useState(null);
  const [outboundMaze, setOutboundMaze] = useState(null);
  const [optimalMaze, setOptimalMaze] = useState(null);
  const [detailLoading, setDetailLoading] = useState(false);
  const [detailError, setDetailError] = useState(null);

  function renderFeedback(message) {
    return (
      <div className={styles.container}>
        <p className={styles.feedback}>{message}</p>
      </div>
    );
  }

  function formatNumber(value, digits) {
    const numericValue = Number(value);
    return Number.isFinite(numericValue) ? numericValue.toFixed(digits) : '--';
  }

  async function handleSelectAttempt(attempt) {
    setSelectedAttempt(attempt);
    setOutboundMaze(null);
    setOptimalMaze(null);
    setDetailError(null);
    setDetailLoading(true);

    try {
      const response = await analisarTentativa(attempt.numtentativa);
      const analysis = response.data;

      if (!analysis?.outboundPath?.length) {
        setDetailError('Essa tentativa ainda não tem trajeto salvo.');
        return;
      }

      setOutboundMaze(analysisToMazeViewProps(analysis, 'outboundPath'));
      setOptimalMaze(analysisToMazeViewProps(analysis, 'optimalPath'));
    } catch (err) {
      setDetailError(err.message);
    } finally {
      setDetailLoading(false);
    }
  }

  useEffect(() => {
    listarHistorico()
      .then((res) => setHistorico(res.data))
      .catch((err) => setError(err.message))
      .finally(() => setLoading(false));
  }, []);

  const filteredHistorico = historico.filter((attempt) => (
    sizeFilter === 'ALL' || attempt.tipolabirinto === sizeFilter
  ));

  if (loading) return renderFeedback('Carregando histórico...');
  if (error) return renderFeedback(`Erro: ${error}`);
  if (historico.length === 0) return renderFeedback('Nenhuma tentativa registrada ainda.');

  return (
    <div className={styles.container}>
      <div className={styles.header}>
        <div>
          <h1 className={styles.title}>Histórico de Tentativas</h1>
          <p className={styles.subtitle}>
            Clique em uma tentativa para comparar o primeiro caminho e o caminho ótimo.
          </p>
        </div>
        <div className={styles.filterField}>
          <span className={styles.filterLabel}>Tamanho</span>
          <div className={styles.filterGroup}>
            {FILTER_OPTIONS.map((option) => (
              <button
                key={option.value}
                type="button"
                className={`${styles.filterChip} ${sizeFilter === option.value ? styles.filterChipActive : ''}`}
                onClick={() => setSizeFilter(option.value)}
              >
                {option.label}
              </button>
            ))}
          </div>
        </div>
      </div>

      <div className={styles.contentGrid}>
        <div className={styles.tableWrapper}>
          <table className={styles.table}>
            <thead>
              <tr>
                <th>Tentativa</th>
                <th>Labirinto</th>
                <th>Vel. Média (m/s)</th>
                <th>Bateria (%)</th>
                <th>Corrente (A)</th>
                <th>Tensão (V)</th>
                <th>Conclusão</th>
                <th>Desafio</th>
              </tr>
            </thead>
            <tbody>
              {filteredHistorico.map((h) => (
                <tr
                  key={h.numtentativa}
                  data-testid={`history-row-${h.numtentativa}`}
                  className={selectedAttempt?.numtentativa === h.numtentativa ? styles.selectedRow : ''}
                  onClick={() => handleSelectAttempt(h)}
                >
                  <td>#{h.numtentativa}</td>
                  <td>{TIPO_LABEL[h.tipolabirinto] ?? h.tipolabirinto}</td>
                  <td>{formatNumber(h.velocidademedia, 2)}</td>
                  <td>{formatNumber(h.percentualbateria, 1)}</td>
                  <td>{formatNumber(h.correnteeletrica, 2)}</td>
                  <td>{formatNumber(h.tensaoeletrica, 2)}</td>
                  <td>{new Date(h.tempoconclusao).toLocaleString('pt-BR')}</td>
                  <td>
                    <span className={h.desafiocumprido === 'SIM' ? styles.good : styles.warn}>
                      {h.desafiocumprido}
                    </span>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
          {filteredHistorico.length === 0 && (
            <div className={styles.emptyState}>
              Nenhuma tentativa encontrada para esse tamanho de labirinto.
            </div>
          )}
        </div>

        <section className={styles.detailPanel}>
          <div className={styles.detailHeader}>
            <h2 className={styles.detailTitle}>
              {selectedAttempt ? `Tentativa #${selectedAttempt.numtentativa}` : 'Análise de Trajetos'}
            </h2>
            {selectedAttempt && (
              <span className={styles.detailMeta}>
                {TIPO_LABEL[selectedAttempt.tipolabirinto] ?? selectedAttempt.tipolabirinto}
              </span>
            )}
          </div>

          {!selectedAttempt && (
            <p className={styles.feedback}>Selecione uma tentativa na tabela para ver os caminhos.</p>
          )}

          {selectedAttempt && detailLoading && (
            <p className={styles.feedback}>Carregando análise da tentativa...</p>
          )}

          {selectedAttempt && !detailLoading && detailError && (
            <p className={styles.feedback}>{detailError}</p>
          )}

          {selectedAttempt && !detailLoading && !detailError && outboundMaze && optimalMaze && (
            <div className={styles.detailBody}>
              <div className={styles.mazeComparison}>
                <div className={styles.mazeCard}>
                  <h3 className={styles.mazeCardTitle}>Primeiro caminho</h3>
                  <p className={styles.mazeCardSubtitle}>Start até o centro 2×2</p>
                  <MazeView
                    grid={outboundMaze.grid}
                    position={outboundMaze.position}
                    goal={outboundMaze.goal}
                    start={outboundMaze.start}
                    visitedPath={outboundMaze.visitedPath}
                    status={outboundMaze.status}
                  />
                </div>
                <div className={styles.mazeCard}>
                  <h3 className={styles.mazeCardTitle}>Caminho ótimo</h3>
                  <p className={styles.mazeCardSubtitle}>Menor entre ida e volta</p>
                  <MazeView
                    grid={optimalMaze.grid}
                    position={optimalMaze.position}
                    goal={optimalMaze.goal}
                    start={optimalMaze.start}
                    visitedPath={optimalMaze.visitedPath}
                    status={optimalMaze.status}
                  />
                </div>
              </div>
            </div>
          )}
        </section>
      </div>
    </div>
  );
}
