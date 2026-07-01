import { execSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { MmsRunner } from "./mms_runner.js";
import { scenarios } from "./scenarios.js";

const __dirname = dirname(fileURLToPath(import.meta.url));
const backendRoot = resolve(__dirname, "../..");
const reportsDir = resolve(backendRoot, "../../reports");
const buildDir = resolve(backendRoot, "build");

const isWindows = process.platform === "win32";
const executableName = isWindows ? "FloodFill.exe" : "FloodFill";
const executablePath = join(buildDir, executableName);

function compileFloodFill() {
  mkdirSync(buildDir, { recursive: true });
  const source = join(backendRoot, "FloodFill.cpp");
  const cmd = isWindows
    ? `g++ -std=c++17 -O2 "${source}" -o "${executablePath}"`
    : `g++ -std=c++17 -O2 "${source}" -o "${executablePath}"`;

  console.log(`Compilando: ${cmd}`);
  execSync(cmd, { stdio: "inherit" });
}

async function main() {
  const filterId = process.argv.find((a) => a.startsWith("--scenario="))?.split("=")[1];
  const selected = filterId ? scenarios.filter((s) => s.id === filterId) : scenarios;

  if (selected.length === 0) {
    console.error(`Cenário não encontrado: ${filterId}`);
    process.exit(1);
  }

  compileFloodFill();

  if (!existsSync(executablePath)) {
    console.error(`Executável não encontrado: ${executablePath}`);
    process.exit(1);
  }

  mkdirSync(reportsDir, { recursive: true });

  const startedAt = new Date().toISOString();
  const results = [];

  for (const scenario of selected) {
    console.log(`\n=== ${scenario.id}: ${scenario.name} ===`);
    const runner = new MmsRunner(executablePath, scenario);
    const result = await runner.run(scenario.timeoutMs ?? 120_000);

    const failedValidations = result.validations.filter((v) => !v.pass);
    if (failedValidations.length > 0 && result.status === "PASS") {
      result.status = "FAIL";
    }

    console.log(`Status: ${result.status} | Passos: ${result.steps} | Fases: ${result.phasesReached.join(", ") || "nenhuma"}`);
    if (result.error) console.log(`Erro: ${result.error}`);

    results.push(result);
  }

  const passed = results.filter((r) => r.status === "PASS").length;
  const report = {
    suite: "MMS Integration — FloodFill.cpp",
    timestamp: startedAt,
    executable: executablePath,
    platform: process.platform,
    summary: {
      total: results.length,
      passed,
      failed: results.length - passed,
    },
    cases: results,
  };

  const jsonPath = join(reportsDir, "mms-integration.json");
  writeFileSync(jsonPath, JSON.stringify(report, null, 2), "utf8");
  console.log(`\nRelatório salvo em: ${jsonPath}`);

  process.exit(passed === results.length ? 0 : 1);
}

main().catch((err) => {
  console.error(err);
  process.exit(1);
});
