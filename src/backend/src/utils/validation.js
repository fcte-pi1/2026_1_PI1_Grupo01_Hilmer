export function toFiniteNumber(value, fallback = 0) {
  const numeric = Number(value);
  return Number.isFinite(numeric) ? numeric : fallback;
}

export function clampBattery(value) {
  const numeric = toFiniteNumber(value, 0);
  return Math.min(100, Math.max(0, numeric));
}
