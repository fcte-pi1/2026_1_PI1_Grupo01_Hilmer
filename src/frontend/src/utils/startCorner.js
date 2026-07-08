export const START_CORNERS = [
  { value: 'top-left', label: 'Superior esquerdo' },
  { value: 'top-right', label: 'Superior direito' },
  { value: 'bottom-left', label: 'Inferior esquerdo' },
  { value: 'bottom-right', label: 'Inferior direito' },
];

export const DEFAULT_START_CORNER = START_CORNERS[0].value;

export function normalizeStartCorner(value) {
  return START_CORNERS.some((corner) => corner.value === value)
    ? value
    : DEFAULT_START_CORNER;
}

export function formatStartCorner(value) {
  return START_CORNERS.find((corner) => corner.value === normalizeStartCorner(value))?.label
    ?? START_CORNERS[0].label;
}

export function mirrorPointForCorner(point, dimension, corner) {
  if (!Array.isArray(point) || point.length < 2) {
    return point;
  }

  const [row, col] = point;
  const lastIndex = dimension - 1;
  const normalizedCorner = normalizeStartCorner(corner);

  switch (normalizedCorner) {
    case 'top-right':
      return [row, lastIndex - col];
    case 'bottom-left':
      return [lastIndex - row, col];
    case 'bottom-right':
      return [lastIndex - row, lastIndex - col];
    default:
      return [row, col];
  }
}

export function mirrorGridForCorner(grid, corner) {
  if (!Array.isArray(grid)) {
    return grid;
  }

  const normalizedCorner = normalizeStartCorner(corner);
  let nextGrid = grid.map((row) => [...row]);

  if (normalizedCorner === 'top-right' || normalizedCorner === 'bottom-right') {
    nextGrid = nextGrid.map((row) => [...row].reverse());
  }

  if (normalizedCorner === 'bottom-left' || normalizedCorner === 'bottom-right') {
    nextGrid = [...nextGrid].reverse();
  }

  return nextGrid;
}
