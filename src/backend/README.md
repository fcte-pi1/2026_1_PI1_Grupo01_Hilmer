# _Backend_

Esta pasta deverá armazenar arquivos referentes a:

- Código-fonte da API REST: rotas, controladores, modelos e lógica de negócio.
- Arquivos de configuração do servidor: `app.py`, `server.js`, `main.go` etc., dependendo da linguagem/framework utilizado ([Flask](https://flask.palletsprojects.com/), [FastAPI](https://fastapi.tiangolo.com/), [Express](https://expressjs.com/), [Django](https://www.djangoproject.com/) etc.).
- Arquivos de definição de dependências: `requirements.txt` ou `pyproject.toml` (Python), `package.json` (Node.js), `pom.xml` (Java/Maven) etc.
- Scripts de migração e esquemas de banco de dados: arquivos `.sql`, scripts de migração ([Alembic](https://alembic.sqlalchemy.org/), [Sequelize](https://sequelize.org/) etc.) e seeds de dados para desenvolvimento.
- Arquivos de configuração de ambiente: `.env.example` com as variáveis de ambiente necessárias (nunca o `.env` real).
- Arquivos de containerização: `Dockerfile` e `docker-compose.yml`, caso o serviço seja executado em contêiner.

Evite incluir:

- Credenciais e segredos: arquivos `.env`, chaves de API, senhas, tokens de acesso ou qualquer dado sensível **nunca** devem ser versionados.
- Artefatos de build: diretórios como `__pycache__/`, `dist/`, `build/`, `.eggs/` devem ser gerados localmente e ignorados via `.gitignore`.
- Dependências instaladas: pastas como `node_modules/` ou ambientes virtuais Python (`venv/`, `.env/`) não devem ser incluídos no repositório.
- Arquivos temporários/específicos do sistema operacional: arquivos gerados automaticamente pelo sistema ou pelo gerenciador de arquivos (ex.: `*~`, `.DS_Store`, `Thumbs.db`).
> [!WARNING]
> **Não acrescente arquivos referentes ao _frontend_ nesta pasta.** Eles deverão ser armazenados na pasta [frontend](https://github.com/fcte-pi1/template/tree/main/src/frontend) deste repositório.

---

# Rodando o Banco de Dados Localmente

## 📋 Pré-requisitos

| Ferramenta | Versão mínima | Download |
|---|---|---|
| Docker Desktop | 4.x | https://www.docker.com/products/docker-desktop |
| Node.js | 18.x | https://nodejs.org |
| npm | 9.x | Incluído com o Node.js |

> ⚠️ O Docker Desktop deve estar **aberto e rodando** antes de qualquer comando.

## Configuração para primeira vez

### 1. Clone o repositório e entre na pasta do backend

```bash
cd src/backend
```

### 2. Crie o arquivo `.env` a partir do exemplo

```bash
cp .env.example .env
```

Edite o `.env` com as credenciais do banco:
 
```env
DB_HOST=localhost
DB_PORT=5432
DB_NAME=micromouse_db
DB_USER=pi1_user
DB_PASSWORD=pi1senha123
```

### 3. Instale as dependências Node

```bash
npm install
```

---

## Subindo o banco

```bash
docker-compose up -d
```

Esse comando:
- Baixa a imagem `postgres:16` (apenas na primeira vez)
- Cria o container `micromouse_postgres`
- Executa o `schema.sql` automaticamente, criando todas as tabelas
- Persiste os dados no volume `pgdata`

Para verificar se está rodando:

```bash
docker ps
```

Você deve ver o container `micromouse_postgres` com status `Up`.

---

## Comandos úteis

| Ação | Comando |
|---|---|
| Subir o banco | `docker-compose up -d` |
| Parar o banco | `docker-compose stop` |
| Ver logs do banco | `docker-compose logs postgres` |
| Acessar o psql (terminal SQL) | `docker exec -it micromouse_postgres psql -U pi1_user -d micromouse_db` |

---

## Resetar o banco do zero

> Use isso quando precisar recriar as tabelas após mudanças no `schema.sql`.

```bash
docker-compose down -v
docker-compose up -d
```

O `-v` destrói o volume `pgdata`, forçando o PostgreSQL a re-executar o `schema.sql` na próxima subida.

---

## Rodando os testes

Com o banco rodando, execute:

```bash
node db.test.js
```

Os testes verificam:

| # | O que testa |
|---|---|
| 1 | Conexão com o banco |
| 2 | Existência das tabelas (`historico`, `telemetria`, `trajeto`) |
| 3 | INSERT em `HISTORICO` |
| 4 | INSERT em `TELEMETRIA` com FK válida |
| 5 | INSERT em `TRAJETO` com múltiplos passos |
| 6 | Rejeição de FK inválida (integridade referencial) |
| 7 | Rejeição de direção inválida (CHECK constraint) |
| 8 | Rejeição de PK composta duplicada em `TRAJETO` |
| 9 | CASCADE DELETE (apagar HISTORICO remove filhos) |

Saída esperada ao passar em tudo:

```
=======================================================
  TESTES - Banco Micromouse PI1
=======================================================

📦 [1] Conexão com o banco
  ✅ PASSOU: Conexão estabelecida com sucesso

...

=======================================================
  Resultado: 9 passaram | 0 falharam
=======================================================
```

---


## ❓ Problemas comuns

**"Cannot connect to the Docker daemon"**
→ Abra o Docker Desktop e aguarde ele inicializar completamente.

**"password authentication failed"**
→ Confira se o `.env` está preenchido corretamente e se o banco foi criado com as mesmas credenciais.

**Tabelas não foram criadas**
→ O `schema.sql` só roda na primeira inicialização. Se o volume já existia, rode `docker-compose down -v` e suba novamente.

**Porta 5432 já em uso**
→ Outro PostgreSQL pode estar rodando na máquina. Altere `DB_PORT` no `.env` e no `docker-compose.yml` para outra porta (ex: `5433`).


---

# Guia de Integração — Micromouse PI1

Mapa do que foi alterado e como rodar tudo junto.

---

## O que mudou

| Arquivo | Situação anterior | Agora |
|---|---|---|
| `backend/src/server.js` | node:http puro + array em memória | Express + PostgreSQL real |
| `backend/src/services/mouseService.js` | Vazio | FloodFill BFS funcional |
| `backend/src/services/simulationService.js` | Não existia | CRUD no banco |
| `backend/src/routes/simulationRoutes.js` | Não existia | GET + POST /api/simulations |
| `backend/src/database/schema.sql` | Sem tabela simulations | Tabela `simulations` criada |
| `frontend/src/services/apiService.js` | Não existia | Todos os fetch centralizados |
| `frontend/src/hooks/useTelemetryData.js` | Só mock | Mock + WebSocket real (por variável de ambiente) |
| `frontend/src/pages/History.jsx` | Vazia/estática | Lê do banco via API |
| `frontend/src/pages/NewAttempt.jsx` | Sem integração | Salva simulação no banco |

---

## Como rodar

### 1. Banco de dados
```bash
# Na raiz do backend
cp .env.example .env       # editar se necessário
docker compose up -d       # sobe o PostgreSQL
```
O schema.sql é executado automaticamente pelo Docker na primeira vez.

### 2. Backend
```bash
cd backend
npm install
npm run dev    # ou: node src/server.js
```

### 3. Frontend
```bash
cd frontend
cp .env.example .env       # ajustar VITE_TELEMETRY_MODE=mock|live
npm install
npm run dev
```

---

## Variáveis de ambiente importantes

### Backend (`backend/.env`)
```
HOST=127.0.0.1
PORT=3001
FRONTEND_ORIGIN=http://localhost:5173
DB_HOST=localhost
DB_PORT=5432
DB_NAME=micromouse_db
DB_USER=pi1_user2
DB_PASSWORD=senhafalsa123
ESP32_WS_URL=ws://192.168.4.1:81   # IP da ESP32 na rede local
```

### Frontend (`frontend/.env`)
```
VITE_API_URL=http://localhost:3001
VITE_WS_URL=ws://localhost:3001
VITE_TELEMETRY_MODE=mock   # trocar para 'live' com hardware
```

---

## TODOs pendentes para integração completa

- [ ] **NewAttempt**: adicionar inputs para `mouseName` e `algorithmUsed`
- [ ] **Dashboard → NewAttempt**: implementar callback de conclusão para salvar simulação automaticamente ao terminar
- [ ] **mouseService**: integrar com `FloodFill.cpp` via WebAssembly ou child_process
- [ ] **ESP32**: validar schema do payload JSON recebido via WebSocket
- [ ] **simulationService**: adicionar paginação no `getAllSimulations()`
- [ ] **schema.sql**: adicionar tabelas `HISTORICO`, `TELEMETRIA`, `TRAJETO` do modelo SBD2
- [ ] **Autenticação**: adicionar middleware de API key ou JWT antes do deploy

---

## Fluxo de dados completo

```
ESP32-C3
   │ WebSocket (ws://192.168.4.1:81)
   ▼
backend/server.js
   │ retransmite via WebSocket
   ▼
frontend/useTelemetryData.js  (modo 'live')
   │ atualiza estado React
   ▼
Dashboard.jsx → MazeView.jsx

── HTTP REST ──────────────────────────────
NewAttempt.jsx
   │ POST /api/simulations
   ▼
simulationRoutes.js → simulationService.js → PostgreSQL

History.jsx
   │ GET /api/simulations
   ▼
simulationRoutes.js → simulationService.js → PostgreSQL
```