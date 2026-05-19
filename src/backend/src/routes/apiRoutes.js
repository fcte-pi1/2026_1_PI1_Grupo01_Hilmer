import { getHealth, getStatus }         from '../controllers/healthController.js';
import { getMazeState, getMazeHistory } from '../controllers/mazeController.js';

export const routes = [
  { method: 'GET', path: '/api/health',        handler: getHealth      },
  { method: 'GET', path: '/api/status',        handler: getStatus      },
  { method: 'GET', path: '/api/maze/state',    handler: getMazeState   },
  { method: 'GET', path: '/api/maze/history',  handler: getMazeHistory },
];