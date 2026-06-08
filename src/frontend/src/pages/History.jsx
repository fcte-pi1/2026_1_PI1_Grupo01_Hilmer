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

  useEffect(() => {
    listarHistorico()
      .then((res) => setHistorico(res.data))
      .catch((err) => setError(err.message))
      .finally(() => setLoading(false));
  }, []);

  if (loading) return <p className={styles.feedback}>Carregando histórico...</p>;
  if (error)   return <p className={styles.feedback}>Erro: {error}</p>;
  if (historico.length === 0)
    return <p className={styles.feedback}>Nenhuma tentativa registrada ainda.</p>;

  return (
    <div className={styles.container}>
      <h1 className={styles.title}>Histórico de Tentativas</h1>

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
              <td>{Number(h.velocidademedia).toFixed(2)}</td>
              <td>{Number(h.percentualbateria).toFixed(1)}</td>
              <td>{Number(h.correnteeletrica).toFixed(2)}</td>
              <td>{Number(h.tensaoeletrica).toFixed(2)}</td>
              <td>{new Date(h.tempoconclusao).toLocaleString('pt-BR')}</td>
              <td className={h.desafiocumprido === 'SIM' ? styles.good : styles.warn}>
                {h.desafiocumprido}
              </td>
            </tr>
          ))}
        </tbody>
      </table>

      {/* TODO: adicionar botão para ver telemetria/trajeto de cada tentativa */}
    </div>
  );
}
