import { NavLink, Outlet } from 'react-router-dom';
import styles from './DefaultLayout.module.css';

export function DefaultLayout() {
  return (
    <div className={styles.shell}>
      <nav className={styles.topNav}>
        <span className={styles.brand}>PI1 · Micromouse</span>
        <div className={styles.navLinks}>
          <NavLink
            to="/dashboard"
            className={({ isActive }) => isActive ? `${styles.link} ${styles.active}` : styles.link}
          >
            Dashboard
          </NavLink>
          <NavLink
            to="/history"
            className={({ isActive }) => isActive ? `${styles.link} ${styles.active}` : styles.link}
          >
            Histórico
          </NavLink>
        </div>
      </nav>
      <main className={styles.content}>
        <Outlet />
      </main>
    </div>
  );
}
