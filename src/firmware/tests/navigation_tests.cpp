#include "navigation_core.h"
#include <iostream>

int tests_run = 0;
int tests_failed = 0;

#define ASSERT_EQ(a,b) do { \
    ++tests_run; \
    if (!((a) == (b))) { \
        ++tests_failed; \
        std::cout << "FAIL: " << __FILE__ << ":" << __LINE__ << " - " #a " != " #b << " (" << (a) << " vs " << (b) << ")\n"; \
    } \
} while(0)

void test_floodfill_basic() {
    MazeMap maze;
    maze.begin();
    Position goal{2,2};
    FloodFillNavigator nav(&maze);
    nav.compute(goal);

    ASSERT_EQ(maze.get(2,2).dist, 0);
    ASSERT_EQ(maze.get(2,3).dist, 1);
    ASSERT_EQ(maze.get(1,2).dist, 1);
    // check further distance
    ASSERT_EQ(maze.get(2,4).dist, 2);
}

void test_navigation_decide_forward() {
    MazeMap maze;
    maze.begin();
    Position goal{1,1};
    FloodFillNavigator nav(&maze);
    nav.compute(goal);

    NavigationController ctrl(&maze);
    Position robot{0,1};
    MoveCommand cmd = ctrl.decide(robot, NORTH);
    ASSERT_EQ(cmd, MOVE_FORWARD);
}

void test_navigation_blocked_forward() {
    MazeMap maze;
    maze.begin();
    // place a wall north of robot at (0,1)
    maze.get(0,1).northWall = true;

    Position goal{1,1};
    FloodFillNavigator nav(&maze);
    nav.compute(goal);

    NavigationController ctrl(&maze);
    Position robot{0,1};
    MoveCommand cmd = ctrl.decide(robot, NORTH);
    // since forward is blocked, expect not MOVE_FORWARD
    ASSERT_EQ(cmd == MOVE_FORWARD, false);
}

void test_maze_initialization_and_borders() {
    MazeMap maze;
    maze.begin();

    // all cells default
    for (int r = 0; r < MAZE_SIZE; ++r) {
        for (int c = 0; c < MAZE_SIZE; ++c) {
            ASSERT_EQ(maze.get(r,c).dist, INF);
            ASSERT_EQ(maze.get(r,c).visited, false);
        }
    }

    // external borders set as in implementation
    for (int i = 0; i < MAZE_SIZE; ++i) {
        ASSERT_EQ(maze.get(0,i).southWall, true);
        ASSERT_EQ(maze.get(MAZE_SIZE-1,i).northWall, true);
        ASSERT_EQ(maze.get(i,0).westWall, true);
        ASSERT_EQ(maze.get(i,MAZE_SIZE-1).eastWall, true);
    }

    // inner cell has no walls by default
    ASSERT_EQ(maze.get(1,1).northWall, false);
    ASSERT_EQ(maze.get(1,1).southWall, false);
    ASSERT_EQ(maze.get(1,1).eastWall, false);
    ASSERT_EQ(maze.get(1,1).westWall, false);
}

void test_valid_positions() {
    MazeMap maze;
    maze.begin();

    ASSERT_EQ(maze.valid(0,0), true);
    ASSERT_EQ(maze.valid(MAZE_SIZE-1, MAZE_SIZE-1), true);
    ASSERT_EQ(maze.valid(-1,0), false);
    ASSERT_EQ(maze.valid(MAZE_SIZE,0), false);
    ASSERT_EQ(maze.valid(0,MAZE_SIZE), false);
}

void test_floodfill_isolation() {
    MazeMap maze;
    maze.begin();

    // isolate cell (3,3) by setting neighbor walls to block entry
    maze.get(2,3).northWall = true; // block (2,3) -> (3,3)
    maze.get(4,3).southWall = true; // block (4,3) -> (3,3)
    maze.get(3,2).eastWall  = true; // block (3,2) -> (3,3)
    maze.get(3,4).westWall  = true; // block (3,4) -> (3,3)

    Position goal{0,0};
    FloodFillNavigator nav(&maze);
    nav.compute(goal);

    ASSERT_EQ(maze.get(3,3).dist, INF);
}

void test_navigation_choose_right() {
    MazeMap maze;
    maze.begin();

    // manually set distances around robot at (5,5)
    maze.get(6,5).dist = 10; // north
    maze.get(5,6).dist = 5;  // east
    maze.get(4,5).dist = 20; // south
    maze.get(5,4).dist = 30; // west

    NavigationController ctrl(&maze);
    Position robot{5,5};
    MoveCommand cmd = ctrl.decide(robot, NORTH);
    ASSERT_EQ(cmd, TURN_RIGHT); // east is best
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
    run_test("test_floodfill_basic", test_floodfill_basic);
    run_test("test_navigation_decide_forward", test_navigation_decide_forward);
    run_test("test_navigation_blocked_forward", test_navigation_blocked_forward);
    run_test("test_maze_initialization_and_borders", test_maze_initialization_and_borders);
    run_test("test_valid_positions", test_valid_positions);
    run_test("test_floodfill_isolation", test_floodfill_isolation);
    run_test("test_navigation_choose_right", test_navigation_choose_right);

    std::cout << "\nSummary: Tests run: " << tests_run << ", failed: " << tests_failed << "\n";
    if (tests_failed == 0) {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    } else {
        std::cout << "SOME TESTS FAILED\n";
        return 1;
    }
}
