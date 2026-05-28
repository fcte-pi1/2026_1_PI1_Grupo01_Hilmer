CC = g++
CFLAGS = -std=c++17 -I. -O2

SRCS = navigation_core.cpp hardware_adapter.cpp
TARGET = navigation_test
NAV_TEST = tests/navigation_tests
MOTOR_SIM_TEST = tests/motor_driver_sim_tests

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

test: $(NAV_TEST) $(MOTOR_SIM_TEST)
	./$(NAV_TEST)
	./$(MOTOR_SIM_TEST)

$(NAV_TEST): tests/navigation_tests.cpp navigation_core.cpp
	$(CC) $(CFLAGS) tests/navigation_tests.cpp navigation_core.cpp -o $(NAV_TEST)

$(MOTOR_SIM_TEST): tests/motor_driver_sim_tests.cpp hardware_adapter.cpp navigation_core.cpp
	$(CC) $(CFLAGS) tests/motor_driver_sim_tests.cpp hardware_adapter.cpp navigation_core.cpp -o $(MOTOR_SIM_TEST)

clean:
	rm -f $(TARGET) $(NAV_TEST) $(MOTOR_SIM_TEST) *.o

.PHONY: all clean test
