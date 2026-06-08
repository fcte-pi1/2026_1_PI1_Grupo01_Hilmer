import { Link } from 'react-router-dom';
import styles from './NotFound.module.css';

export function NotFound() {
  return (
    <div className={styles.page}>
      <span className={styles.code}>404</span>
      <h1 className={styles.title}>Página não encontrada</h1>
      <p className={styles.subtitle}>O caminho que você tentou não existe no labirinto.</p>
      <Link to="/dashboard" className={styles.link}>← Voltar ao Dashboard</Link>
    </div>
  );
}
