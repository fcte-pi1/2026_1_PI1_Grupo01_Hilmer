#ifdef TEST_MODE
/**
 * Firmware de teste para Wokwi (ESP32-WROOM-32D / esp32dev).
 * Imprime marcadores parseáveis na Serial para automação.
 */
#include <Arduino.h>
#include "../navigation/floodfill_engine.h"
#include "../memory/maze_memory.h"
#include "../config/config.h"

static int testsPassed = 0;
static int testsFailed = 0;

static void runTest(const char *id, const char *name, bool pass, int expected, int actual)
{
    if (pass)
    {
        testsPassed++;
        Serial.printf("TEST_PASS %s name=%s expected=%d actual=%d\n", id, name, expected, actual);
    }
    else
    {
        testsFailed++;
        Serial.printf("TEST_FAIL %s name=%s expected=%d actual=%d\n", id, name, expected, actual);
    }
}

static void testUt01(void)
{
    Serial.println("TEST_START UT01");
    floodfill_init();
    floodfill_complete('C');
    int mid = MAZE_SIZE / 2;
    bool pass = (manhattan_dist[0][0] == 14) && (manhattan_dist[mid - 1][mid - 1] == 0);
    runTest("UT01", "center_distances_empty", pass, 14, manhattan_dist[0][0]);
    Serial.println("TEST_END UT01");
}

static void testUt02(void)
{
    Serial.println("TEST_START UT02");
    floodfill_init();
    floodfill_complete('S');
    bool pass = (manhattan_dist[0][0] == 0) && (manhattan_dist[15][15] == 30);
    runTest("UT02", "start_target_empty", pass, 0, manhattan_dist[0][0]);
    Serial.println("TEST_END UT02");
}

static void testUt03(void)
{
    Serial.println("TEST_START UT03");
    floodfill_init();
    floodfill_complete('C');
    horiz_walls[5][5] = true;
    floodfill_queue_reset();
    floodfill_queue_push(4, 5);
    floodfill_queue_push(5, 5);
    floodfill_modified('C');
    bool pass = manhattan_dist[4][5] == floodfill_get_min_neighbor(4, 5) + 1;
    runTest("UT03", "modified_after_wall", pass, 1, manhattan_dist[4][5]);
    Serial.println("TEST_END UT03");
}

void setup(void)
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("TEST_SUITE_START floodfill_esp32");
    Serial.println("BOARD esp32-wroom-32d");

    testUt01();
    testUt02();
    testUt03();

    Serial.printf("TEST_SUITE_END passed=%d failed=%d\n", testsPassed, testsFailed);
}

void loop(void)
{
    delay(1000);
}
#endif
