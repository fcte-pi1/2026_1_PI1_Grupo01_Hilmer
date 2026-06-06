import { createServer } from "node:http";

const port = Number(process.env.PORT || 3001);
const host = process.env.HOST || "127.0.0.1";
const frontendOrigin = process.env.FRONTEND_ORIGIN || "http://localhost:5173";

const simulationDatabase = [];

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
        backend: "Node.js ativo",
        database: "pendente de definicao",
        sensors: "pendente de definicao"
      });
      return;
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

server.listen(port, host, () => {
  console.log(`[backend] API escutando em http://${host}:${port}`);
});