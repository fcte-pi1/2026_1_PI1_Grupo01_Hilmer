CC = g++
CFLAGS = -std=c++17 -I. -O2

SRCS = navigation_core.cpp hardware_adapter.cpp
TARGET = navigation_test

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

test: navigation_tests
	./navigation_tests


navigation_tests: tests/navigation_tests.cpp navigation_core.cpp
	$(CC) $(CFLAGS) tests/navigation_tests.cpp navigation_core.cpp -o tests/navigation_tests

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
