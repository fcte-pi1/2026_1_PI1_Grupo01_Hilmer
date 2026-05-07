# Template PI1

Esse é o _template_ de repositório para ser utilizado pelos grupos de PI1 para organizar seu projeto. O _template_ é dividido em pastas, onde cada parte do projeto deve ser armazenada. Os arquivos a serem armazenados incluem documentação, código-fonte, arquivos de CAD, esquemáticos, arquivos de simulação de circuitos, e dados.

A organização e a correta utilização do repositório do projeto serão considerados na avaliação do grupo. Dessa forma, recomenda-se que *todos os membros* do grupo leiam as instruções deste repositório, aprendam a a utilizar o git (caso ainda não saibam) e também que o grupo combine uma estratégia de como irão utilizar o repositório em conjunto. Dessa forma não deixem de utilizar todas as ferramentas que o GitHub oferece, incluindo branchs, PRs, revisões, issues, calendários, dentre outros.

Lembrem sempre de evitar enviar arquivos muito grandes (>5MB). No caso de vídeos e outros arquivos pesados que são necessários, armazenar o arquivo em outra plataforma e colocar aqui apenas o _link_.

> [!IMPORTANT]
> A estrutura de pastas do projeto não reflete a divisão de equipes. Os membros podem e devem trabalhar nas diferentes pastas a depender da necessidade do projeto.

## Utilização

1. Crie o repositório do projeto utilizando a nomenclatura padrão no formato: `<ano>.<semestre>_PI1_Grupo<n>_<professor>`. Como um exemplo, um nome formado corretamente seria `2026.1_PI1_Grupo1_Diogo`. 

2. Crie uma equipe do projeto com a mesma nomenclatura do repositório porém com o sufixo `_Equipe`, como `2026.1_PI1_Grupo1_Diogo_Equipe`, e solicite, caso necessário, que a equipe tenha permissão de escrita no repositório do projeto.

## Configuração de ambiente

O script [scripts/setup_dev_env.sh](/home/lucasmf/projects/PI1/2026_1_PI1_Grupo01_Hilmer/scripts/setup_dev_env.sh:1) prepara o ambiente de desenvolvimento levando em conta o estado atual do projeto:

- frontend em `React + Vite`
- backend em `Node.js`
- banco de dados ainda pendente de definição
- comunicação com sensores por `HTTP`, com hardware ainda pendente de definição
- auditoria da estrutura atual do repositório ao final da execução

Para instalar dependências e validar o ambiente:

```bash
bash scripts/setup_dev_env.sh install
```

Esse modo instala apenas as dependências do projeto, com base nos `package.json` de `src/backend` e `src/frontend`.

Para instalar pacotes de sistema necessários ao ambiente:

```bash
bash scripts/setup_dev_env.sh install-system
```

Para apenas checar o ambiente atual, sem instalar nada:

```bash
bash scripts/setup_dev_env.sh check
```

Para iniciar frontend e backend automaticamente:

```bash
bash scripts/start_dev.sh
```

Esse script sobe os dois serviços juntos e encerra ambos ao usar `Ctrl+C`.

O script tenta instalar:

- dependências de `src/backend` quando existir `package.json`
- dependências de `src/frontend` quando existir `package.json`

No modo `install-system`, o script tenta instalar:

- `git`
- `curl`
- utilitários básicos de compilação do sistema

Observações:

- O modo `install` não usa `sudo` nem instala pacotes do sistema.
- Em Linux/macOS, o modo `install-system` pode pedir `sudo`.
- O script usa `npm ci` quando existir `package-lock.json`; caso contrário, usa `npm install`.
- O script copia `.env.example` para `.env` em `src/backend` e `src/frontend` quando esses arquivos existirem.
- O banco de dados e o hardware de comunicação com sensores ainda precisam ser definidos pelo time.
