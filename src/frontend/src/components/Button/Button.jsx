import styles from './Button.module.css';

export function Button({ children, onClick, variant = 'primary', disabled = false, fullWidth = false }) {
  return (
    <button
      className={[
        styles.btn,
        styles[variant],
        fullWidth ? styles.fullWidth : '',
      ].join(' ')}
      onClick={onClick}
      disabled={disabled}
    >
      {children}
    </button>
  );
}
