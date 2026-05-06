#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FRONTEND_DIR="$ROOT_DIR/src/frontend"
BACKEND_DIR="$ROOT_DIR/src/backend"

frontend_pid=""
backend_pid=""

log() {
    printf '[dev-start] %s\n' "$1"
}

die() {
    printf '[dev-start] erro: %s\n' "$1" >&2
    exit 1
}

project_has_external_dependencies() {
    local target_dir="$1"

    node -e '
        const fs = require("fs");
        const pkg = JSON.parse(fs.readFileSync(process.argv[1], "utf8"));
        const dependencies = {
          ...(pkg.dependencies || {}),
          ...(pkg.devDependencies || {})
        };
        process.exit(Object.keys(dependencies).length > 0 ? 0 : 1);
    ' "$target_dir/package.json"
}

project_dependencies_ready() {
    local target_dir="$1"

    if [[ ! -f "$target_dir/package.json" ]]; then
        return 1
    fi

    if [[ -d "$target_dir/node_modules" ]]; then
        return 0
    fi

    if project_has_external_dependencies "$target_dir"; then
        return 1
    fi

    return 0
}

ensure_project_ready() {
    [[ -f "$FRONTEND_DIR/package.json" ]] || die "src/frontend/package.json nao encontrado"
    [[ -f "$BACKEND_DIR/package.json" ]] || die "src/backend/package.json nao encontrado"
    project_dependencies_ready "$FRONTEND_DIR" || die "dependencias do frontend nao instaladas; rode: bash scripts/setup_dev_env.sh install"
    project_dependencies_ready "$BACKEND_DIR" || die "dependencias do backend nao instaladas; rode: bash scripts/setup_dev_env.sh install"
}

cleanup() {
    if [[ -n "$backend_pid" ]] && kill -0 "$backend_pid" 2>/dev/null; then
        kill "$backend_pid" 2>/dev/null || true
    fi

    if [[ -n "$frontend_pid" ]] && kill -0 "$frontend_pid" 2>/dev/null; then
        kill "$frontend_pid" 2>/dev/null || true
    fi
}

start_backend() {
    log "subindo backend em src/backend"
    (
        cd "$BACKEND_DIR"
        npm run dev
    ) &
    backend_pid=$!
}

start_frontend() {
    log "subindo frontend em src/frontend"
    (
        cd "$FRONTEND_DIR"
        npm run dev
    ) &
    frontend_pid=$!
}

main() {
    ensure_project_ready
    trap cleanup EXIT INT TERM

    start_backend
    start_frontend

    log "frontend esperado em http://localhost:5173"
    log "backend esperado em http://localhost:3001"
    log "pressione Ctrl+C para encerrar os dois processos"

    wait -n "$backend_pid" "$frontend_pid"
}

main "$@"
