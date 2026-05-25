# Testes de firmware

Este diretório contém testes unitários para a navegação e uma simulação em host para os drivers dos motores.

Requisitos:
- `g++` (compiler C++ com suporte a C++17) ou `make` em um ambiente Unix/WSL/MSYS2.

Como compilar e executar (usando Makefile do diretório `src/firmware`):

```bash
cd src/firmware
make test
```

Ou compilação manual (Windows ou sem `make`):

```bash
cd src/firmware
g++ -std=c++17 -I. tests/navigation_tests.cpp navigation_core.cpp -o tests/navigation_tests
g++ -std=c++17 -I. tests/motor_driver_sim_tests.cpp hardware_adapter.cpp navigation_core.cpp -o tests/motor_driver_sim_tests
tests/navigation_tests
tests/motor_driver_sim_tests
```

O que os testes cobrem:
- Inicialização correta do `MazeMap` e configuração das paredes externas.
- Validação de posições válidas e inválidas (`MazeMap::valid`).
- Funcionamento do algoritmo Flood Fill (inclui caso de célula isolada).
- Decisões do `NavigationController` em vários cenários (avanço, bloqueio, escolha de direção).
- Validação dos drivers de motor em modo host, registrando comandos emitidos e sequências de GPIO.

Como adicionar testes:
- Crie novas funções de teste em `navigation_tests.cpp` seguindo o padrão existente e chame-as em `main()`.
- Recompile e execute usando `make test` ou o comando `g++` acima.

Para os drivers de motor, use `tests/motor_driver_sim_tests.cpp` como referência. Esse binário roda sem ESP32 conectada e valida automaticamente os comandos emitidos pelo `MotorController`.

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
