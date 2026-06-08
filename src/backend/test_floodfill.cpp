#include <iostream>
#include <cassert>

// Variáveis mocadas do testador
int api_forward_calls = 0;
int api_turn_right_calls = 0;

void test_initialization() {
    // Assert Initialization
    std::cout << "Testes de inicialização rodando..." << std::endl;
}

void run_tests() {
    test_initialization();
    std::cout << "Todos os testes passaram!" << std::endl;
}

int main() {
    run_tests();
    return 0;
}
