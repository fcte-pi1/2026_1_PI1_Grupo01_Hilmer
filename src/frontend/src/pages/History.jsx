/**
 * History.jsx
 *
 * Lista o histórico de tentativas da tabela HISTORICO.
 */

import { useEffect, useState } from 'react';
import { listarHistorico } from '../services/apiService';
import styles from './History.module.css';

const TIPO_LABEL = { '4x4': '4×4', '8x8': '8×8', '16x16': '16×16' };

export function History() {
  const [historico, setHistorico] = useState([]);
  const [loading, setLoading]     = useState(true);
  const [error, setError]         = useState(null);

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

  useEffect(() => {
    listarHistorico()
      .then((res) => setHistorico(res.data))
      .catch((err) => setError(err.message))
      .finally(() => setLoading(false));
  }, []);

  if (loading) return renderFeedback('Carregando histórico...');
  if (error)   return renderFeedback(`Erro: ${error}`);
  if (historico.length === 0)
    return renderFeedback('Nenhuma tentativa registrada ainda.');

  return (
    <div className={styles.container}>
      <h1 className={styles.title}>Histórico de Tentativas</h1>

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
            {historico.map((h) => (
              <tr key={h.numtentativa}>
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
      </div>

      {/* TODO: adicionar botão para ver telemetria/trajeto de cada tentativa */}
    </div>
  );
}
