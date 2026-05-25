#include "hardware_adapter.h"

#include <iostream>
#include <string>
#include <vector>

int tests_run = 0;
int tests_failed = 0;

#define ASSERT_EQ(a,b) do { \
    ++tests_run; \
    if (!((a) == (b))) { \
        ++tests_failed; \
        std::cout << "FAIL: " << __FILE__ << ":" << __LINE__ << " - " #a " != " #b << " (" << (a) << " vs " << (b) << ")\n"; \
    } \
} while(0)

static void assert_writes(const std::vector<firmware_sim::GpioWrite>& writes, const std::vector<firmware_sim::GpioWrite>& expected) {
    ASSERT_EQ(writes.size(), expected.size());
    for (size_t i = 0; i < writes.size() && i < expected.size(); ++i) {
        ASSERT_EQ(writes[i].pin, expected[i].pin);
        ASSERT_EQ(writes[i].value, expected[i].value);
    }
}

static void test_forward() {
    firmware_sim::reset();
    MotorController motor;

    motor.forward();

    ASSERT_EQ(firmware_sim::getMotorActions().size(), 1u);
    ASSERT_EQ(firmware_sim::getMotorActions()[0], std::string("forward"));
    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 1}, {26, 0}, {27, 1}, {14, 0}
    });
}

static void test_turn_left() {
    firmware_sim::reset();
    MotorController motor;

    motor.turnLeft();

    ASSERT_EQ(firmware_sim::getMotorActions().size(), 1u);
    ASSERT_EQ(firmware_sim::getMotorActions()[0], std::string("turnLeft"));
    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 0}, {26, 1}, {27, 1}, {14, 0}
    });
}

static void test_turn_right() {
    firmware_sim::reset();
    MotorController motor;

    motor.turnRight();

    ASSERT_EQ(firmware_sim::getMotorActions().size(), 1u);
    ASSERT_EQ(firmware_sim::getMotorActions()[0], std::string("turnRight"));
    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 1}, {26, 0}, {27, 0}, {14, 1}
    });
}

static void test_backward() {
    firmware_sim::reset();
    MotorController motor;

    motor.backward();

    ASSERT_EQ(firmware_sim::getMotorActions().size(), 1u);
    ASSERT_EQ(firmware_sim::getMotorActions()[0], std::string("backward"));
    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 0}, {26, 1}, {27, 0}, {14, 1}
    });
}

static void test_stop() {
    firmware_sim::reset();
    MotorController motor;

    motor.stop();

    ASSERT_EQ(firmware_sim::getMotorActions().size(), 1u);
    ASSERT_EQ(firmware_sim::getMotorActions()[0], std::string("stop"));
    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 0}, {26, 0}, {27, 0}, {14, 0}
    });
}

static void test_execute_turn_left() {
    firmware_sim::reset();
    MotorController motor;

    motor.execute(TURN_LEFT);

    std::vector<std::string> actions = firmware_sim::getMotorActions();
    ASSERT_EQ(actions.size(), 3u);
    ASSERT_EQ(actions[0], std::string("turnLeft"));
    ASSERT_EQ(actions[1], std::string("delay(220)"));
    ASSERT_EQ(actions[2], std::string("forward"));

    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 0}, {26, 1}, {27, 1}, {14, 0},
        {25, 1}, {26, 0}, {27, 1}, {14, 0}
    });
}

static void test_execute_turn_right() {
    firmware_sim::reset();
    MotorController motor;

    motor.execute(TURN_RIGHT);

    std::vector<std::string> actions = firmware_sim::getMotorActions();
    ASSERT_EQ(actions.size(), 3u);
    ASSERT_EQ(actions[0], std::string("turnRight"));
    ASSERT_EQ(actions[1], std::string("delay(220)"));
    ASSERT_EQ(actions[2], std::string("forward"));

    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 1}, {26, 0}, {27, 0}, {14, 1},
        {25, 1}, {26, 0}, {27, 1}, {14, 0}
    });
}

static void test_execute_turn_back() {
    firmware_sim::reset();
    MotorController motor;

    motor.execute(TURN_BACK);

    std::vector<std::string> actions = firmware_sim::getMotorActions();
    ASSERT_EQ(actions.size(), 3u);
    ASSERT_EQ(actions[0], std::string("turnRight"));
    ASSERT_EQ(actions[1], std::string("delay(450)"));
    ASSERT_EQ(actions[2], std::string("forward"));

    assert_writes(firmware_sim::getDigitalWrites(), {
        {25, 1}, {26, 0}, {27, 0}, {14, 1},
        {25, 1}, {26, 0}, {27, 1}, {14, 0}
    });
}

static void run_test(const char* name, void(*fn)()) {
    std::cout << "RUNNING: " << name << "\n";
    int before_failed = tests_failed;
    int before_run = tests_run;
    fn();
    int after_failed = tests_failed;
    int after_run = tests_run;
    int assertions = after_run - before_run;
    if (after_failed == before_failed) {
        std::cout << "PASS: " << name << " (" << assertions << " assertions)\n";
    } else {
        std::cout << "FAIL: " << name << " (" << assertions << " assertions, " << (after_failed - before_failed) << " failures)\n";
    }
}

int main() {
    run_test("test_forward", test_forward);
    run_test("test_turn_left", test_turn_left);
    run_test("test_turn_right", test_turn_right);
    run_test("test_backward", test_backward);
    run_test("test_stop", test_stop);
    run_test("test_execute_turn_left", test_execute_turn_left);
    run_test("test_execute_turn_right", test_execute_turn_right);
    run_test("test_execute_turn_back", test_execute_turn_back);

    std::cout << "\nSummary: Tests run: " << tests_run << ", failed: " << tests_failed << "\n";
    if (tests_failed == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }

    std::cout << "SOME TESTS FAILED\n";
    return 1;
}