import { afterAll, beforeAll, describe, expect, it } from 'vitest';
import pool from '../../src/database/connection.js';
import { seedAnalysisAttemptViaHttp } from '../fixtures/analysisAttempt.js';
import { waitForApi } from '../helpers/waitForApi.js';

const API_BASE = 'http://127.0.0.1:3001';

let analiseTentativa;

describe('GET /api/historico/:numTentativa/analise (integração)', () => {
  beforeAll(async () => {
    await waitForApi(API_BASE);
  });

  afterAll(async () => {
    if (analiseTentativa) {
      await pool.query('DELETE FROM HISTORICO WHERE numTentativa = $1', [analiseTentativa]);
    }
  });

  it('analisa tentativa com trajeto e telemetria via GET /analise', async () => {
    analiseTentativa = await seedAnalysisAttemptViaHttp(API_BASE);

    const response = await fetch(`${API_BASE}/api/historico/${analiseTentativa}/analise`);
    expect(response.status).toBe(200);

    const body = await response.json();
    expect(body.success).toBe(true);
    expect(body.data.outboundPath.length).toBeGreaterThan(0);
    expect(body.data.optimalPath.length).toBeGreaterThan(0);

    const flat = body.data.grid.flat();
    expect(flat).toContain(1);
    expect(body.data.grid.length).toBe(4);
  });
});
