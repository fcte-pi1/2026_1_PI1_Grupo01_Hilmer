#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OS="$(uname -s)"
MODE="${1:-install}"
NODE_MIN_MAJOR=18

BASE_PACKAGES=()

log() {
    printf '[env-setup] %s\n' "$1"
}

warn() {
    printf '[env-setup] aviso: %s\n' "$1"
}

die() {
    printf '[env-setup] erro: %s\n' "$1" >&2
    exit 1
}

has_command() {
    command -v "$1" >/dev/null 2>&1
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

require_mode() {
    case "$MODE" in
        install|check|install-system)
            ;;
        *)
            die "modo invalido: '$MODE'. Use 'install', 'check' ou 'install-system'."
            ;;
    esac
}

detect_package_manager() {
    if has_command apt-get; then
        printf 'apt'
        return
    fi

    if has_command dnf; then
        printf 'dnf'
        return
    fi

    if has_command pacman; then
        printf 'pacman'
        return
    fi

    if has_command brew; then
        printf 'brew'
        return
    fi

    printf 'unknown'
}

configure_package_lists() {
    local manager="$1"

    case "$manager" in
        apt)
            BASE_PACKAGES=(git curl build-essential)
            ;;
        dnf)
            BASE_PACKAGES=(git curl gcc-c++ make)
            ;;
        pacman)
            BASE_PACKAGES=(git curl base-devel)
            ;;
        brew)
            BASE_PACKAGES=(git curl)
            ;;
        *)
            BASE_PACKAGES=()
            ;;
    esac
}

run_install_command() {
    local manager="$1"

    case "$manager" in
        apt)
            sudo apt-get update
            sudo apt-get install -y "${BASE_PACKAGES[@]}"
            ;;
        dnf)
            sudo dnf install -y "${BASE_PACKAGES[@]}"
            ;;
        pacman)
            sudo pacman -Sy --noconfirm "${BASE_PACKAGES[@]}"
            ;;
        brew)
            brew install "${BASE_PACKAGES[@]}"
            ;;
        *)
            die "gerenciador de pacotes nao suportado para instalacao automatica"
            ;;
    esac
}

ensure_dir_structure() {
    mkdir -p \
        "$ROOT_DIR/src/backend" \
        "$ROOT_DIR/src/frontend" \
        "$ROOT_DIR/src/firmware"
}

copy_env_file() {
    local target_dir="$1"
    local example_file="$target_dir/.env.example"
    local env_file="$target_dir/.env"

    if [[ -f "$example_file" && ! -f "$env_file" ]]; then
        cp "$example_file" "$env_file"
        log "arquivo .env criado em ${target_dir#$ROOT_DIR/}"
    fi
}

check_node_version() {
    local raw_version major

    if ! has_command node; then
        warn "Node.js nao encontrado"
        return 1
    fi

    raw_version="$(node -v | sed 's/^v//')"
    major="${raw_version%%.*}"

    if [[ "$major" -lt "$NODE_MIN_MAJOR" ]]; then
        warn "Node.js $raw_version encontrado; recomendado usar Node.js ${NODE_MIN_MAJOR}+"
        return 1
    fi

    log "ok: Node.js $raw_version encontrado"
    return 0
}

install_npm_dependencies() {
    local target_dir="$1"
    local label="$2"

    if [[ ! -f "$target_dir/package.json" ]]; then
        warn "package.json nao encontrado em ${target_dir#$ROOT_DIR/}; instalacao de dependencias do $label ignorada"
        return
    fi

    if ! has_command npm; then
        warn "npm nao encontrado; instalacao de dependencias do $label ignorada"
        return
    fi

    copy_env_file "$target_dir"

    if [[ -f "$target_dir/package-lock.json" ]]; then
        log "instalando dependencias do $label com npm ci"
        (
            cd "$target_dir"
            npm ci
        )
    else
        log "instalando dependencias do $label com npm install"
        (
            cd "$target_dir"
            npm install
        )
    fi
}

install_project_dependencies() {
    ensure_dir_structure
    copy_env_file "$ROOT_DIR/src/backend"
    copy_env_file "$ROOT_DIR/src/frontend"

    check_required_commands || true
    install_npm_dependencies "$ROOT_DIR/src/backend" "backend"
    install_npm_dependencies "$ROOT_DIR/src/frontend" "frontend"
    report_tool_versions
    audit_repository
    print_next_steps
}

check_required_commands() {
    local missing=0
    local cmd

    for cmd in git curl; do
        if has_command "$cmd"; then
            log "ok: comando '$cmd' encontrado"
        else
            warn "faltando comando obrigatorio: $cmd"
            missing=1
        fi
    done

    if has_command node; then
        check_node_version || missing=1
    else
        warn "faltando Node.js"
        missing=1
    fi

    if has_command npm; then
        log "ok: comando 'npm' encontrado"
    else
        warn "faltando comando 'npm'"
        missing=1
    fi

    return "$missing"
}

report_tool_versions() {
    log "resumo de versoes do ambiente"
    if has_command git; then
        git --version | sed 's/^/[env-setup] /'
    fi

    if has_command node; then
        node --version | sed 's/^/[env-setup] node /'
    fi

    if has_command npm; then
        npm --version | sed 's/^/[env-setup] npm /'
    fi
}

audit_repository() {
    log "auditando estrutura do projeto"

    if [[ -f "$ROOT_DIR/src/frontend/package.json" ]]; then
        log "frontend detectado em src/frontend"
    else
        warn "nenhum frontend React/Vite detectado em src/frontend"
    fi

    if [[ -f "$ROOT_DIR/src/backend/package.json" ]]; then
        log "backend Node.js detectado em src/backend"
    else
        warn "nenhum backend Node.js detectado em src/backend"
    fi

    if [[ -f "$ROOT_DIR/src/frontend/package.json" ]] && grep -q '"vite"' "$ROOT_DIR/src/frontend/package.json"; then
        log "frontend com Vite identificado"
    else
        warn "Vite ainda nao foi confirmado em src/frontend/package.json"
    fi

    if project_dependencies_ready "$ROOT_DIR/src/frontend"; then
        log "dependencias do frontend parecem instaladas"
    else
        warn "dependencias do frontend ainda nao foram instaladas"
    fi

    if project_dependencies_ready "$ROOT_DIR/src/backend"; then
        log "dependencias do backend parecem instaladas"
    else
        warn "dependencias do backend ainda nao foram instaladas"
    fi

    if [[ -f "$ROOT_DIR/src/backend/.env.example" ]]; then
        log ".env.example detectado no backend"
    else
        warn "src/backend/.env.example ainda nao existe"
    fi

    if [[ -f "$ROOT_DIR/src/frontend/.env.example" ]]; then
        log ".env.example detectado no frontend"
    else
        warn "src/frontend/.env.example ainda nao existe"
    fi

    if find "$ROOT_DIR/src/firmware" -maxdepth 3 -type f \( -name '*.ino' -o -name '*.c' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' -o -name 'platformio.ini' -o -name 'CMakeLists.txt' -o -name 'Makefile' \) | grep -q .; then
        log "artefatos de firmware detectados em src/firmware"
    else
        warn "hardware/protocolo com sensores ainda nao foram materializados em src/firmware"
    fi
}

print_next_steps() {
    cat <<'EOF'
[env-setup] proximos passos recomendados:
[env-setup] 1. Instalar as dependencias com: bash scripts/setup_dev_env.sh install
[env-setup] 2. Subir frontend e backend juntos com: bash scripts/start_dev.sh
[env-setup] 3. Ou subir o backend com: cd src/backend && npm run dev
[env-setup] 4. E o frontend com: cd src/frontend && npm run dev
[env-setup] 5. Definir o banco de dados oficial do projeto.
[env-setup] 6. Definir o hardware e o protocolo HTTP entre sensores e backend.
EOF
}

install_mode() {
    install_project_dependencies
}

install_system_mode() {
    local manager

    manager="$(detect_package_manager)"
    configure_package_lists "$manager"

    log "sistema operacional detectado: $OS"
    log "gerenciador de pacotes detectado: $manager"

    if [[ "$manager" == "unknown" ]]; then
        warn "nao foi possivel detectar um gerenciador de pacotes suportado"
    else
        run_install_command "$manager"
    fi

    check_required_commands || true
    report_tool_versions
}

check_mode() {
    ensure_dir_structure
    check_required_commands || true
    report_tool_versions
    audit_repository
    print_next_steps
}

main() {
    require_mode

    case "$MODE" in
        install)
            install_mode
            ;;
        install-system)
            install_system_mode
            ;;
        check)
            check_mode
            ;;
    esac
}

main "$@"
