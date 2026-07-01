import { describe, it, expect } from 'vitest';
import { analysisToMazeViewProps } from '../../src/services/telemetryService';

describe('analysisToMazeViewProps', () => {
  it('monta props do MazeView a partir da análise', () => {
    const analysis = {
      grid: [
        [1, 1, 1, 1],
        [1, 0, 2, 1],
        [1, 2, 2, 1],
        [1, 1, 1, 1],
      ],
      start: [0, 0],
      goal: [1, 1],
      outboundPath: [
        [0, 0],
        [0, 1],
      ],
      optimalPath: [[0, 0]],
    };

    const props = analysisToMazeViewProps(analysis, 'outboundPath');
    expect(props.visitedPath).toEqual([
      [0, 0],
      [0, 1],
    ]);
    expect(props.position).toEqual([0, 1]);
    expect(props.status).toBe('running');

    const optimalProps = analysisToMazeViewProps(analysis, 'optimalPath');
    expect(optimalProps.status).toBe('success');
  });
});
