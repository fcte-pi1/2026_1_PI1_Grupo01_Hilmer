export function formatTime(seconds) {
  const m = Math.floor(seconds / 60).toString().padStart(2, '0');
  const s = (seconds % 60).toString().padStart(2, '0');
  return `${m}:${s}`;
}

export function formatSpeed(metersPerSecond) {
  return `${metersPerSecond.toFixed(2)} m/s`;
}

export function formatBattery(percent) {
  return `${percent}%`;
}

export function formatMazeDimension(size) {
  return `${size}x${size}`;
}

export function mazeSizeToTipoLabirinto(size) {
  if (size <= 12) return '4x4';
  if (size <= 16) return '8x8';
  return '16x16';
}

export function statusLabel(status) {
  const labels = {
    running: 'EM ANDAMENTO',
    success: 'SUCESSO',
    failure: 'FALHA',
    idle: 'AGUARDANDO',
  };
  return labels[status] ?? status;
}
