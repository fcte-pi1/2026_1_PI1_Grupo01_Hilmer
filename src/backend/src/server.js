import { createServer } from "node:http";
import { existsSync, mkdirSync, readFileSync, writeFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { WebSocketServer, WebSocket } from "ws";

const port = Number(process.env.PORT || 3001);
const host = process.env.HOST || "127.0.0.1";
const frontendOrigin = process.env.FRONTEND_ORIGIN || "http://localhost:5173";
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const runtimeDataDir = join(__dirname, "..", "runtime-data");
const firstPassStateFilePath = join(runtimeDataDir, "first-pass-status.json");

const simulationDatabase = [];
let esp32Connection = null;
let activeConfiguration = null;
const firstPassStatusByMaze = new Map();

function loadFirstPassStateFromDisk() {
  try {
    if (!existsSync(firstPassStateFilePath)) return;

    const raw = readFileSync(firstPassStateFilePath, "utf-8");
    const parsed = JSON.parse(raw);

    if (!parsed || typeof parsed !== "object" || Array.isArray(parsed)) return;

    for (const [mazeSizeKey, status] of Object.entries(parsed)) {
      const mazeSize = Number(mazeSizeKey);
      if (!Number.isFinite(mazeSize)) continue;
      if (!status || typeof status !== "object") continue;

      firstPassStatusByMaze.set(mazeSize, {
        started: status.started === true,
        completed: status.completed === true,
        updatedAt: typeof status.updatedAt === "string" ? status.updatedAt : new Date().toISOString()
      });
    }
  } catch (error) {
    logOperation("WARN", "Falha ao carregar estado de primeira passagem do disco", { error: error.message });
  }
}

function persistFirstPassStateToDisk() {
  try {
    mkdirSync(runtimeDataDir, { recursive: true });

    const serialized = Object.fromEntries(firstPassStatusByMaze.entries());
    writeFileSync(firstPassStateFilePath, JSON.stringify(serialized, null, 2), "utf-8");
  } catch (error) {
    logOperation("WARN", "Falha ao persistir estado de primeira passagem no disco", { error: error.message });
  }
}

function parseMazeSizeFromLabel(label) {
  if (typeof label !== "string") return null;

  const match = label.match(/^(\d+)x\1$/);
  if (!match) return null;

  return Number(match[1]);
}

function isFirstPassCompletedTelemetry(payload) {
  const completionFlag = payload?.desafioCumprido;
  return completionFlag === true || completionFlag === "S" || completionFlag === "s" || completionFlag === "true";
}

function sendJson(response, statusCode, payload) {
  response.writeHead(statusCode, {
    "Content-Type": "application/json; charset=utf-8",
    "Access-Control-Allow-Origin": frontendOrigin,
    "Access-Control-Allow-Methods": "GET,POST,OPTIONS", // Adicionado POST para permitir envios
    "Access-Control-Allow-Headers": "Content-Type"
  });

  response.end(JSON.stringify(payload));
}

// MIDDLEWARE/AUXILIAR: Registro estruturado de logs (Atende CA-43.6)
function logOperation(level, message, detail = "") {
  const timestamp = new Date().toISOString();
  console.log(`[${timestamp}] [${level}] ${message} ${detail ? `| Detalhes: ${JSON.stringify(detail)}` : ""}`);
}

// HELPER: Captura e processa o corpo da requisição de forma assíncrona
function getRequestBody(request) {
  return new Promise((resolve, reject) => {
    let body = "";
    request.on("data", (chunk) => { body += chunk; });
    request.on("end", () => {
      try {
        resolve(body ? JSON.parse(body) : {});
      } catch (err) {
        reject(new Error("JSON inválido enviado no corpo da requisição."));
      }
    });
  });
}

function sendToESP32(payload) {
  if (!esp32Connection || esp32Connection.readyState !== WebSocket.OPEN) {
    return false;
  }

  esp32Connection.send(JSON.stringify(payload));
  return true;
}

const server = createServer(async (request, response) => {
  const url = new URL(request.url || "/", `http://${request.headers.host}`);
  if (request.method === "OPTIONS") {
    response.writeHead(204, {
      "Access-Control-Allow-Origin": frontendOrigin,
      "Access-Control-Allow-Methods": "GET,POST,OPTIONS",
      "Access-Control-Allow-Headers": "Content-Type"
    });
    response.end();
    return;
  }

  try {
    if (request.method === "GET" && url.pathname === "/api/health") {
      sendJson(response, 200, {
        status: "online",
        message: "API base do projeto disponível.",
        runtime: "node-http",
        pending: { database: "nao_definido", sensors: "nao_definido" },
        timestamp: new Date().toISOString()
      });
      return;
    }

    if (request.method === "GET" && url.pathname === "/api/status") {
      sendJson(response, 200, {
        frontend: "esperado em React + Vite",
        backend: "Node.js ativo (HTTP + WebSockets)",
        database: "pendente de definicao",
        sensors: "pendente de definicao"
      });
      return;
    }

    if (request.method === "POST" && url.pathname === "/api/micromouse/configuracoes") {
      const body = await getRequestBody(request);
      const mazeSize = Number(body.mazeSize);
      const run = Number(body.run);

      if (![10, 12, 14, 16, 18, 20].includes(mazeSize)) {
        return sendJson(response, 400, { success: false, error: "Tamanho do labirinto inválido." });
      }

      if (![1, 2].includes(run)) {
        return sendJson(response, 400, { success: false, error: "A execução deve ser 1 ou 2." });
      }

      if (run === 2) {
        const firstPassStatus = firstPassStatusByMaze.get(mazeSize);

        if (!firstPassStatus?.completed) {
          return sendJson(response, 409, {
            success: false,
            error: `A segunda passagem para ${mazeSize}x${mazeSize} só é liberada após concluir a primeira.`
          });
        }
      }

      const configurationPayload = {
        tipoLabirinto: `${mazeSize}x${mazeSize}`,
        mazeSize,
        run,
        execucao: run === 1 ? "primeira" : "segunda"
      };

      const delivered = sendToESP32(configurationPayload);

      if (!delivered) {
        return sendJson(response, 503, {
          success: false,
          error: "A ESP32 não está conectada para receber a configuração."
        });
      }

      activeConfiguration = configurationPayload;

      if (run === 1) {
        const previousStatus = firstPassStatusByMaze.get(mazeSize);
        firstPassStatusByMaze.set(mazeSize, {
          started: true,
          completed: previousStatus?.completed === true,
          updatedAt: new Date().toISOString()
        });
        persistFirstPassStateToDisk();
      }

      logOperation("INFO", "Configuração enviada ao Micromouse", configurationPayload);

      return sendJson(response, 200, {
        success: true,
        message: "Configurações enviadas ao Micromouse.",
        data: configurationPayload
      });
    }

    // CRITÉRIO CA-43.1: Endpoint POST para salvar dados e processar regras do Micromouse
    if (request.method === "POST" && url.pathname === "/api/simulations") {
      const body = await getRequestBody(request);
      const { mouseName, algorithmUsed, mazeSize, executionTimeSeconds } = body;

      // CRITÉRIO CA-43.5: Validação estrita dos dados recebidos antes do processamento
      if (!mouseName || typeof mouseName !== "string" || mouseName.trim().length < 2) {
        logOperation("WARN", "Tentativa de cadastro com nome inválido", body);
        return sendJson(response, 400, { success: false, error: "Nome do mouse inválido." });
      }
      if (!executionTimeSeconds || typeof executionTimeSeconds !== "number" || executionTimeSeconds <= 0) {
        return sendJson(response, 400, { success: false, error: "O tempo deve ser um número maior que zero." });
      }

      // CRITÉRIO CA-43.2: Implementação de Regra de Negócio (Cálculo de Eficiência do algoritmo)
      const efficiencyScore = parseFloat(((mazeSize || 16) * (mazeSize || 16) / executionTimeSeconds).toFixed(2));
      const performanceStatus = efficiencyScore > 5 ? "Alta Performance" : "Ajustes Necessários";

      // CRITÉRIO CA-43.3 & CA-43.7: Operação CRUD (Create) sem inconsistência
      const record = {
        id: simulationDatabase.length + 1,
        mouseName,
        algorithmUsed: algorithmUsed || "Não Especificado",
        mazeSize: mazeSize || 16,
        executionTimeSeconds,
        efficiencyScore,
        performanceStatus,
        createdAt: new Date().toISOString()
      };

      simulationDatabase.push(record);

      // CRITÉRIO CA-43.6: Registro de logs básicos de operação bem-sucedida
      logOperation("INFO", `Nova simulação salva com sucesso. ID: ${record.id}`);

      // CRITÉRIO CA-43.4: Resposta padronizada
      return sendJson(response, 201, { success: true, message: "Simulação registrada.", data: record });
    }

    // CRITÉRIO CA-43.1 & CA-43.3: Endpoint GET (Read do CRUD) para listar o histórico
    if (request.method === "GET" && url.pathname === "/api/simulations") {
      logOperation("INFO", "Leitura do histórico de simulações requisitada.");
      return sendJson(response, 200, { success: true, data: simulationDatabase });
    }

    sendJson(response, 404, { error: "Rota nao encontrada" });

  } catch (error) {
    // CRITÉRIO CA-43.6: Registro estrito de Logs de Erros inesperados
    logOperation("ERROR", `Falha no processamento: ${error.message}`);
    sendJson(response, 500, { success: false, error: "Erro interno no servidor do Micromouse." });
  }
});

const wssReact = new WebSocketServer({ server });

wssReact.on("connection", (ws) => {
  console.log("[backend] Frontend React conectado ao WebSocket.");
  ws.on("close", () => console.log("[backend] Frontend React desconectado."));
});

function connectToESP32() {
  const esp32Url = process.env.ESP32_WS_URL || "ws://192.168.4.1:81";
  console.log(`[backend] Tentando conectar na ESP32 em ${esp32Url}...`);
  const esp32Ws = new WebSocket(esp32Url);
  esp32Connection = esp32Ws;

  esp32Ws.on("open", () => {
    console.log("[backend] Conectado com sucesso à ESP32-C3!");
  });

  esp32Ws.on("message", (data) => {
    const rawMessage = data.toString();

    try {
      const telemetryPayload = JSON.parse(rawMessage);
      const telemetryMazeSize = parseMazeSizeFromLabel(telemetryPayload?.tipoLabirinto);
      const activeMazeSize = activeConfiguration?.mazeSize;
      const mazeSize = telemetryMazeSize ?? activeMazeSize;

      if (mazeSize && activeConfiguration?.run === 1 && isFirstPassCompletedTelemetry(telemetryPayload)) {
        const previousStatus = firstPassStatusByMaze.get(mazeSize);
        if (!previousStatus?.completed) {
          firstPassStatusByMaze.set(mazeSize, {
            started: true,
            completed: true,
            updatedAt: new Date().toISOString()
          });
          persistFirstPassStateToDisk();

          logOperation("INFO", "Primeira passagem concluída e liberada para segunda", { mazeSize });
        }
      }
    } catch {
      // Mensagem sem formato JSON esperado; apenas repassa ao frontend.
    }

    wssReact.clients.forEach((client) => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(rawMessage);
      }
    });
  });

  esp32Ws.on("close", () => {
    console.log("[backend] Conexão com a ESP32 perdida. Tentando reconectar em 2 segundos...");
    if (esp32Connection === esp32Ws) {
      esp32Connection = null;
    }
    setTimeout(connectToESP32, 2000);
  });

  esp32Ws.on("error", (err) => {
    console.error("[backend] Erro na conexão com a ESP32:", err.message);
  });
}

if (process.env.NODE_ENV !== 'test') {
  loadFirstPassStateFromDisk();
  connectToESP32();
}

server.listen(port, host, () => {
  console.log(`[backend] API e WebSocket escutando em http://${host}:${port}`);
});

export { server, wssReact, connectToESP32 };
