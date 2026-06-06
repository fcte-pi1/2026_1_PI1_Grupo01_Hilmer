import { useNavigate } from 'react-router-dom';
import styles from './Home.module.css';

export function Home() {
  const navigate = useNavigate();

  return (
    <div className={styles.container}>
      <div className={styles.grid} aria-hidden="true">
        {Array.from({ length: 64 }).map((_, i) => (
          <div key={i} className={styles.cell} />
        ))}
      </div>

      <div className={styles.content}>
        <div className={styles.badge}>MICROMOUSE</div>

        <h1 className={styles.title}>
          Labirinto
          <br />
          <span className={styles.titleAccent}>do rato</span>
        </h1>

        <p className={styles.subtitle}>
          Controle e monitore o percurso do rato em tempo real
        </p>

        <div className={styles.actions}>
          <button
            className={styles.btnPrimary}
            onClick={() => navigate('/new-attempt')}
          >
            <span className={styles.btnIcon}>▶</span>
            Nova tentativa
          </button>

          <button
            className={styles.btnSecondary}
            onClick={() => navigate('/history')}
          >
            Consultar tentativas
          </button>
        </div>
      </div>

      <div className={styles.corner} aria-hidden="true">
        <div className={styles.cornerDot} />
        <div className={styles.cornerDot} />
        <div className={styles.cornerDot} />
      </div>
    </div>
  );
}
