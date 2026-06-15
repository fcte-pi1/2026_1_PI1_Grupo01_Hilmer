// MOCK PAGE — dados históricos são carregados de dataService.js com dados mockados.
// A integração real com a API do backend será feita futuramente.

import { useEffect, useState } from 'react';
import { Card } from '../components/Card/Card';
import { ExecutionCard } from '../components/ExecutionCard/ExecutionCard';
import { StatusBadge } from '../components/StatusBadge/StatusBadge';
import { getExecutionHistory } from '../services/dataService';
import { Filter } from "../components/Filter/Filter";
import {
  formatBattery,
  formatMazeDimension,
  formatSpeed,
  formatTime,
  filterExecutions
} from '../utils/helpers';
import styles from './History.module.css';

export function History() {
  const [executions, setExecutions] = useState([]);
  const [selected, setSelected] = useState(null);
  const [isFilterOpen, setIsFilterOpen] = useState(false);
  const [filters, setFilters] = useState({
    mazeSize: [],
    totalTime: [],
    avgSpeed: [],
    totalBatteryUsed: [],
    status: [],
  });

  useEffect(() => {
    getExecutionHistory().then(setExecutions);
  }, []);

  const handleSelect = (execution) => {
    setSelected((prev) => (prev?.id === execution.id ? null : execution));
  };

  const filteredExecutions = filterExecutions(executions, filters);

  return (
    <div className={styles.page}>
    <div className={styles.header}>
      <h1 className={styles.title}>Histórico de Execuções</h1>
      <span className="mock-banner">
        DADOS MOCKADOS
      </span>
      <button
        className={styles.filterButton}
        onClick={() => setIsFilterOpen(true)}
      >
        Filtros
      </button>
    </div>
      <div className={styles.layout}>
        <div className={styles.list}>
          {filteredExecutions.map((exec) => (
            <ExecutionCard
              key={exec.id}
              execution={exec}
              selected={selected?.id === exec.id}
              onClick={() => handleSelect(exec)}
            />
          ))}
        </div>

        <aside className={styles.detail}>
          {selected ? (
            <Card>
              <div className={styles.detailHeader}>
                <h2 className={styles.detailTitle}>Tentativa #{selected.attempt}</h2>
                <StatusBadge status={selected.status} />
              </div>
              <dl className={styles.detailGrid}>
                <div className={styles.detailRow}>
                  <dt>Dimensão</dt>
                  <dd>{formatMazeDimension(selected.mazeSize)}</dd>
                </div>
                <div className={styles.detailRow}>
                  <dt>Tempo total</dt>
                  <dd>{selected.status === 'failure' ? '—' : formatTime(selected.totalTimeSeconds)}</dd>
                </div>
                <div className={styles.detailRow}>
                  <dt>Velocidade média</dt>
                  <dd>{selected.status === 'failure' ? '—' : formatSpeed(selected.avgSpeedMps)}</dd>
                </div>
                <div className={styles.detailRow}>
                  <dt>Consumo total</dt>
                  <dd>{formatBattery(selected.totalBatteryUsed)}</dd>
                </div>
              </dl>
              {selected.status === 'failure' && (
                <p className={styles.failureNote}>
                  O micromouse não concluiu o percurso nesta tentativa.
                </p>
              )}
            </Card>
          ) : (
            <div className={styles.emptyDetail}>
              <p>Selecione uma execução para ver os detalhes.</p>
            </div>
          )}
        </aside>
        <Filter
          isOpen={isFilterOpen}
          onClose={() => setIsFilterOpen(false)}
          filters={filters}
          setFilters={setFilters}
        />
      </div>
    </div>
  );
}
