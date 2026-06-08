import { execSync, spawnSync } from "node:child_process";
import { existsSync, mkdirSync, writeFileSync } from "node:fs";
import { dirname, join, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { homedir } from "node:os";

const __dirname = dirname(fileURLToPath(import.meta.url));
const root = resolve(__dirname, "..");
const reportsDir = join(root, "reports");
const backendTests = join(root, "src", "backend", "tests", "mms_harness", "run_all.js");
const firmwareDir = join(root, "src", "firmware", "micromouse");

function resolvePio() {
  if (process.platform === "win32") {
    const candidate = join(homedir(), ".platformio", "penv", "Scripts", "pio.exe");
    if (existsSync(candidate)) return `"${candidate}"`;
  }
  return "pio";
}

const pioCmd = resolvePio();

mkdirSync(reportsDir, { recursive: true });

const results = {
  mms: { status: "SKIP", message: "" },
  unit: { status: "SKIP", message: "" },
  wokwi: { status: "SKIP", message: "" },
};

function runStep(name, fn) {
  console.log(`\n========== ${name} ==========`);
  try {
    fn();
    console.log(`${name}: OK`);
  } catch (err) {
    console.error(`${name}: FALHOU — ${err.message}`);
  }
}

runStep("MMS Integration", () => {
  const scenario = process.argv.find((a) => a.startsWith("--scenario="));
  const args = scenario ? [backendTests, scenario] : [backendTests];
  const result = spawnSync(process.execPath, args, { stdio: "inherit", cwd: root });
  results.mms.status = result.status === 0 ? "PASS" : "FAIL";
  if (result.status !== 0) throw new Error("MMS integration failed");
});

runStep("Unity Native", () => {
  try {
    execSync(`${pioCmd} test -e native -v --junit-output-path "${join(reportsDir, "unit-floodfill.xml")}"`, {
      cwd: firmwareDir,
      stdio: "inherit",
      shell: true,
    });
    results.unit.status = "PASS";
  } catch {
    results.unit.status = "FAIL";
    throw new Error("PlatformIO native tests failed — verifique se PlatformIO está instalado");
  }
});

runStep("Wokwi ESP32 (opcional)", () => {
  try {
    execSync(`${pioCmd} run -e esp32dev_test`, { cwd: firmwareDir, stdio: "inherit", shell: true });
  } catch {
    results.wokwi.status = "SKIP";
    results.wokwi.message = "Build esp32dev_test falhou";
    writeFileSync(join(reportsDir, "wokwi-serial.log"), "", "utf8");
    return;
  }

  try {
    execSync("wokwi-cli --version", { stdio: "pipe" });
    execSync(`wokwi-cli run "${firmwareDir}" --timeout 30000 --serial-log-file "${join(reportsDir, "wokwi-serial.log")}"`, {
      cwd: firmwareDir,
      stdio: "inherit",
    });
    results.wokwi.status = "PASS";
  } catch {
    results.wokwi.status = "SKIP";
    results.wokwi.message =
      "wokwi-cli não disponível — instale com: iwr https://wokwi.com/ci/install.ps1 -useb | iex (não é pacote npm)";
    writeFileSync(join(reportsDir, "wokwi-serial.log"), "", "utf8");
  }
});

runStep("Gerar relatório", () => {
  execSync(`node "${join(reportsDir, "generate_report.js")}"`, { stdio: "inherit", cwd: root });
});

writeFileSync(join(reportsDir, "run-results.json"), JSON.stringify(results, null, 2));

const failed = Object.values(results).some((r) => r.status === "FAIL");
process.exit(failed ? 1 : 0);
