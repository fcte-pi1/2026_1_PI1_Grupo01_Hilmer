import { statusLabel } from '../../utils/helpers';
import styles from './StatusBadge.module.css';

export function StatusBadge({ status }) {
  return (
    <span className={`${styles.badge} ${styles[status]}`}>
      {statusLabel(status)}
    </span>
  );
}
