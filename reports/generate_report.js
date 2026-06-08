import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";

const __dirname = dirname(fileURLToPath(import.meta.url));
const reportsDir = __dirname;

function readJson(path) {
  if (!existsSync(path)) return null;
  return JSON.parse(readFileSync(path, "utf8"));
}

function parseWokwiLog(logPath) {
  if (!existsSync(logPath)) {
    return { suite: "Wokwi ESP32", cases: [], summary: { total: 0, passed: 0, failed: 0 } };
  }

  const text = readFileSync(logPath, "utf8");
  const lines = text.split(/\r?\n/);
  const cases = [];

  for (const line of lines) {
    const passMatch = line.match(/^TEST_PASS (\w+) name=(\S+) expected=(-?\d+) actual=(-?\d+)/);
    const failMatch = line.match(/^TEST_FAIL (\w+) name=(\S+) expected=(-?\d+) actual=(-?\d+)/);
    if (passMatch) {
      cases.push({
        id: passMatch[1],
        name: passMatch[2],
        status: "PASS",
        expected: Number(passMatch[3]),
        actual: Number(passMatch[4]),
      });
    } else if (failMatch) {
      cases.push({
        id: failMatch[1],
        name: failMatch[2],
        status: "FAIL",
        expected: Number(failMatch[3]),
        actual: Number(failMatch[4]),
      });
    }
  }

  const passed = cases.filter((c) => c.status === "PASS").length;
  return {
    suite: "Wokwi ESP32-WROOM-32D",
    cases,
    summary: { total: cases.length, passed, failed: cases.length - passed },
  };
}

function parseJUnitXml(xmlPath) {
  if (!existsSync(xmlPath)) {
    return { suite: "Unity Native (PlatformIO)", cases: [], summary: { total: 0, passed: 0, failed: 0 } };
  }

  const xml = readFileSync(xmlPath, "utf8");
  const cases = [];

  const testsuiteRegex = /<testsuite\s+name="([^"]*)"[^>]*tests="(\d+)"[^>]*>/g;
  let suiteMatch;
  while ((suiteMatch = testsuiteRegex.exec(xml)) !== null) {
    const suiteName = suiteMatch[1];
    const testCount = Number(suiteMatch[2]);
    if (!suiteName.startsWith("native:") || testCount === 0) continue;

    const suiteStart = suiteMatch.index;
    const suiteEnd = xml.indexOf("</testsuite>", suiteStart);
    const suiteBlock = xml.slice(suiteStart, suiteEnd);

    const testcaseRegex = /<testcase\s+([^>]*?)\/>/g;
    let tcMatch;
    while ((tcMatch = testcaseRegex.exec(suiteBlock)) !== null) {
      const attrs = tcMatch[1];
      const name = attrs.match(/name="([^"]*)"/)?.[1] ?? "unknown";
      const status = attrs.match(/status="([^"]*)"/)?.[1] ?? "PASSED";
      const time = attrs.match(/time="([^"]*)"/)?.[1];
      const file = attrs.match(/file="([^"]*)"/)?.[1];

      cases.push({
        id: name,
        name: `${suiteName.replace(/^native:/, "")} · ${name}`,
        status: status === "PASSED" ? "PASS" : "FAIL",
        time: time ? Number(time) : null,
        file,
      });
    }
  }

  const passed = cases.filter((c) => c.status === "PASS").length;
  return {
    suite: "Unity Native (PlatformIO)",
    cases,
    summary: { total: cases.length, passed, failed: cases.length - passed },
  };
}

function escapeHtml(text) {
  return String(text)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function renderSection(title, data) {
  if (!data || data.summary.total === 0) {
    return `<section><h2>${escapeHtml(title)}</h2><p class="muted">Nenhum resultado disponível.</p></section>`;
  }

  const rows = data.cases
    .map(
      (c) => `
    <tr class="${c.status === "PASS" ? "pass" : "fail"}">
      <td>${escapeHtml(c.id ?? c.name)}</td>
      <td>${escapeHtml(c.name ?? "")}</td>
      <td><span class="badge ${c.status === "PASS" ? "ok" : "bad"}">${c.status}</span></td>
      <td>${c.expected !== undefined ? escapeHtml(c.expected) : c.steps !== undefined ? escapeHtml(c.steps + " passos") : "—"}</td>
      <td>${c.actual !== undefined ? escapeHtml(c.actual) : c.phasesReached?.length ? escapeHtml(c.phasesReached.join(", ")) : c.time != null ? escapeHtml(c.time + "s") : "—"}</td>
      <td>${c.error ? escapeHtml(c.error) : "—"}</td>
    </tr>`
    )
    .join("");

  return `
<section>
  <h2>${escapeHtml(title)}</h2>
  <p>Total: ${data.summary.total} | Passou: ${data.summary.passed} | Falhou: ${data.summary.failed}</p>
  <table>
    <thead><tr><th>ID</th><th>Nome</th><th>Status</th><th>Esperado</th><th>Atual</th><th>Erro</th></tr></thead>
    <tbody>${rows}</tbody>
  </table>
</section>`;
}

function main() {
  mkdirSync(reportsDir, { recursive: true });

  const mms = readJson(join(reportsDir, "mms-integration.json"));
  const unit = parseJUnitXml(join(reportsDir, "unit-floodfill.xml"));
  const wokwi = parseWokwiLog(join(reportsDir, "wokwi-serial.log"));

  const sections = [
    mms ? { title: "Integração MMS — FloodFill.cpp", data: { cases: mms.cases, summary: mms.summary } } : null,
    { title: unit.suite, data: unit },
    { title: wokwi.suite, data: wokwi },
  ].filter(Boolean);

  const totalTests = sections.reduce((acc, s) => acc + s.data.summary.total, 0);
  const totalPassed = sections.reduce((acc, s) => acc + s.data.summary.passed, 0);
  const totalFailed = totalTests - totalPassed;
  const timestamp = new Date().toISOString();

  const html = `<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8" />
  <title>Relatório de Testes — FloodFill</title>
  <style>
    body { font-family: Segoe UI, sans-serif; margin: 2rem; background: #f6f8fa; color: #1f2328; }
    h1 { margin-bottom: 0.2rem; }
    .meta { color: #656d76; margin-bottom: 2rem; }
    section { background: #fff; border: 1px solid #d0d7de; border-radius: 8px; padding: 1rem 1.2rem; margin-bottom: 1.5rem; }
    table { width: 100%; border-collapse: collapse; margin-top: 0.8rem; }
    th, td { border-bottom: 1px solid #d0d7de; padding: 0.55rem 0.6rem; text-align: left; font-size: 0.92rem; }
    .badge { padding: 0.15rem 0.5rem; border-radius: 999px; font-size: 0.75rem; font-weight: 600; }
    .badge.ok { background: #dafbe1; color: #116329; }
    .badge.bad { background: #ffebe9; color: #82071e; }
    .summary { display: flex; gap: 1rem; flex-wrap: wrap; }
    .card { background: #fff; border: 1px solid #d0d7de; border-radius: 8px; padding: 1rem 1.2rem; min-width: 140px; }
    .card strong { display: block; font-size: 1.6rem; }
    .muted { color: #656d76; }
    tr.pass td { background: #f6ffed; }
    tr.fail td { background: #fff5f5; }
  </style>
</head>
<body>
  <h1>Relatório de Testes — FloodFill</h1>
  <p class="meta">Gerado em ${escapeHtml(timestamp)}</p>
  <div class="summary">
    <div class="card"><span>Total</span><strong>${totalTests}</strong></div>
    <div class="card"><span>Passou</span><strong>${totalPassed}</strong></div>
    <div class="card"><span>Falhou</span><strong>${totalFailed}</strong></div>
  </div>
  ${sections.map((s) => renderSection(s.title, s.data)).join("\n")}
</body>
</html>`;

  const summary = {
    timestamp,
    totals: { total: totalTests, passed: totalPassed, failed: totalFailed },
    suites: sections.map((s) => ({ title: s.title, summary: s.data.summary })),
    mms,
    unit,
    wokwi,
  };

  writeFileSync(join(reportsDir, "summary.html"), html, "utf8");
  writeFileSync(join(reportsDir, "summary.json"), JSON.stringify(summary, null, 2), "utf8");
  writeFileSync(join(reportsDir, "run-results.json"), JSON.stringify({
    timestamp,
    mms: mms?.summary ?? null,
    unit: unit.summary,
    wokwi: wokwi.summary,
  }, null, 2), "utf8");

  console.log(`Relatório HTML: ${join(reportsDir, "summary.html")}`);
  console.log(`Relatório JSON: ${join(reportsDir, "summary.json")}`);
  console.log(`Run results: ${join(reportsDir, "run-results.json")}`);
}

main();
