// Stress test concurrent pour WebServ, cible webserv.conf (port 8090).
// Lancement: k6 run tests/k6/stress.js
// Params ajustables via variables d'env: VUS, ITERATIONS, BASE_URL
import http from 'k6/http';
import { check, group } from 'k6';

const BASE_URL = __ENV.BASE_URL || 'http://127.0.0.1:8090';

export const options = {
  scenarios: {
    stress: {
      executor: 'shared-iterations',
      vus: Number(__ENV.VUS) || 20,
      iterations: Number(__ENV.ITERATIONS) || 2000,
      maxDuration: '5m',
    },
  },
};

// corps > client_max_body_size (50M sur /uploads dans webserv.conf) -> doit
// etre rejete en 413, pas faire planter/hang le serveur.
const OVERSIZED_BODY = 'A'.repeat(55 * 1024 * 1024);
const NORMAL_BODY = 'B'.repeat(1024); // 1K, sous la limite

export default function () {
  const scenario = Math.random();

  if (scenario < 0.6) {
    group('GET /', () => {
      const res = http.get(`${BASE_URL}/`);
      check(res, {
        'GET / -> 200': (r) => r.status === 200,
      });
    });
  } else if (scenario < 0.75) {
    group('GET /cgi-bin/serve.py', () => {
      const res = http.get(`${BASE_URL}/cgi-bin/serve.py`);
      check(res, {
        'GET /cgi-bin/serve.py -> 200': (r) => r.status === 200,
      });
    });
  } else if (scenario < 0.9) {
    group('POST /uploads (normal)', () => {
      const res = http.post(`${BASE_URL}/uploads/`, NORMAL_BODY, {
        headers: { 'Content-Type': 'text/plain' },
      });
      check(res, {
        'POST /uploads normal -> pas de crash reseau': (r) => r.status !== 0,
        'POST /uploads normal -> 2xx/4xx propre': (r) => r.status >= 200 && r.status < 500,
      });
    });
  } else {
    group('POST /uploads (oversized > 50M)', () => {
      const res = http.post(`${BASE_URL}/uploads/`, OVERSIZED_BODY, {
        headers: { 'Content-Type': 'text/plain' },
        timeout: '30s',
      });
      check(res, {
        'POST /uploads oversized -> 413 (pas de hang/crash)': (r) => r.status === 413,
      });
    });
  }
}
