# Testes de `navigation_core`

Este diretório contém testes unitários simples para o módulo `navigation_core` (algoritmo Flood Fill e decisão de navegação).

Requisitos:
- `g++` (compiler C++ com suporte a C++17) ou `make` em um ambiente Unix/WSL/MSYS2.

Como compilar e executar (usando Makefile do diretório `src/firmware`):

```bash
cd src/firmware
make test
```

Ou compilação manual (Windows ou sem `make`):

```bash
g++ -std=c++17 -I src/firmware -O2 src/firmware/tests/navigation_tests.cpp src/firmware/navigation_core.cpp -o src/firmware/tests/navigation_tests
src/firmware/tests/navigation_tests
```

O que os testes cobrem:
- Inicialização correta do `MazeMap` e configuração das paredes externas.
- Validação de posições válidas e inválidas (`MazeMap::valid`).
- Funcionamento do algoritmo Flood Fill (inclui caso de célula isolada).
- Decisões do `NavigationController` em vários cenários (avanço, bloqueio, escolha de direção).

Como adicionar testes:
- Crie novas funções de teste em `navigation_tests.cpp` seguindo o padrão existente e chame-as em `main()`.
- Recompile e execute usando `make test` ou o comando `g++` acima.

Saída esperada:
- O runner agora imprime cada teste individualmente. Para cada caso você verá linhas como:
	- `RUNNING: <test_name>` — início do teste
	- `PASS: <test_name> (<M> assertions)` — se passou
	- `FAIL: <test_name> (<M> assertions, <K> failures)` — se falhou

- No final é mostrado um sumário:
	- `Summary: Tests run: N, failed: K`
	- `ALL TESTS PASSED` ou `SOME TESTS FAILED`

Exemplo de saída (quando tudo passa):

```
RUNNING: test_floodfill_basic
PASS: test_floodfill_basic (4 assertions)
...
Summary: Tests run: 593, failed: 0
ALL TESTS PASSED
```
