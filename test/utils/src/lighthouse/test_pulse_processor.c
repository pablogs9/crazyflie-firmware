#include "pulse_processor.h"

#include <stdlib.h>
#include <string.h>
#include "unity.h"

#include "mock_ootx_decoder.h"
#include "mock_lighthouse_calibration.h"

#define FRAME_LENGTH 400000    // 8.333ms
#define SWEEP_MAX_WIDTH 1024    // 20us
#define SWEEP_CENTER 192000    // 4ms
#define SYNC_BASE_WIDTH 2750
#define SYNC_DIVIDER 500
#define SYNC_SEPARATION 19200
#define MAX_FRAME_LENGTH_NOISE 400

#define SYNC_X SYNC_BASE_WIDTH+(SYNC_DIVIDER * 0)
#define SYNC_Y SYNC_BASE_WIDTH+(SYNC_DIVIDER * 1)
#define SYNC_X_SKIP SYNC_BASE_WIDTH+(SYNC_DIVIDER * 4)
#define SYNC_Y_SKIP SYNC_BASE_WIDTH+(SYNC_DIVIDER * 5)
#define SWEEP 512
#define LONG_SWEEP 1500


static void assertSyncTimeIsMultipleOfFrameLength(uint32_t expectedSyncTime, uint32_t actualSyncTime);
static void limitTimestamps(pulseProcessorPulse_t history[]);

// Functions under test
bool findSyncTime(const pulseProcessorPulse_t pulseHistory[], uint32_t *foundSyncTime);
bool getSystemSyncTime(const uint32_t syncTimes[], size_t nSyncTimes, uint32_t *syncTime);


void setUp(void) {
}

void testThatFindSyncCanDetectSync0FromTwoBasestations()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+SYNC_SEPARATION},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatFindSyncCanDetectSync0FromTwoBasestationsMissaligned()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = 2*FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (-1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (-1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + ( 0*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime + ( 0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + ( 0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + ( 1*FRAME_LENGTH)+0},
    {.width = SYNC_X,      .timestamp = expectedSyncTime + ( 1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + ( 1*FRAME_LENGTH)+SWEEP_CENTER},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatFindSyncCanDetectSync0FromOneBasestation()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (3*FRAME_LENGTH)+0},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (3*FRAME_LENGTH)+SWEEP_CENTER},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatFindSyncCanDetectSync0FromTwoBasestationsWithShortSpuriousSpike()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER+100},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatFindSyncFailsWhenReceivingFromTwoBasestationsWithShortSpuriousSpikeBetweenSyncs()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+(SYNC_SEPARATION/2)},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_FALSE(found);
}

void testThatFindSyncCanDetectSync0FromTwoBasestationsWithNoisyPulseLength()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X + 10,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SYNC_X_SKIP - 10, .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP - 10,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y +  7,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP - 10, .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP + 10,       .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP -  8, .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
    {.width = SYNC_X +  4,      .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+SYNC_SEPARATION},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatFindSyncFailsWhenReceivingFromTwoBasestationsWithLongSweep()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = LONG_SWEEP,  .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = LONG_SWEEP,  .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+SYNC_SEPARATION},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_FALSE(found);
}

void testThatFindSyncCanDetectSync0FromTwoBasestationsWithTimingNoise()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = FRAME_LENGTH;
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + 10 + (0*FRAME_LENGTH)+0},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime -  3 + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime +  4 + (0*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_Y,      .timestamp = expectedSyncTime +  7 + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime -  9 + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + 10 + (1*FRAME_LENGTH)+SWEEP_CENTER},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime - 10 + (2*FRAME_LENGTH)+0},
    {.width = SYNC_X,      .timestamp = expectedSyncTime +  3 + (2*FRAME_LENGTH)+SYNC_SEPARATION},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatFindSyncCanDetectSync0FromTwoBasestationsWithTimestampWrapping()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = (1<<PULSE_PROCESSOR_TIMESTAMP_BITWIDTH) - ((1*FRAME_LENGTH)+SYNC_SEPARATION);
  pulseProcessorPulse_t pulseHistory[PULSE_PROCESSOR_HISTORY_LENGTH] =  {
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+0},
    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (0*FRAME_LENGTH)+SWEEP_CENTER},

    {.width = SYNC_Y,      .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+0},
    {.width = SYNC_Y_SKIP, .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SYNC_SEPARATION},
    {.width = SWEEP,       .timestamp = expectedSyncTime + (1*FRAME_LENGTH)+SWEEP_CENTER},

    {.width = SYNC_X_SKIP, .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+0},
    {.width = SYNC_X,      .timestamp = expectedSyncTime + (2*FRAME_LENGTH)+SYNC_SEPARATION},
  };
  limitTimestamps(pulseHistory);

  // Test
  bool found = findSyncTime(pulseHistory, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatGetSystemSyncTimeReturnsTheAverageForGoodSyncData()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = 2;
  uint32_t syncTimes[8] = {1, 2, 3};
  size_t nSyncTimes = 3;

  // Test
  bool found = getSystemSyncTime(syncTimes, nSyncTimes, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL(expectedSyncTime, actualSyncTime);
}

void testThatGetSystemSyncTimeHandlesTimestampsFromMultipleFrames()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = 2;
  uint32_t syncTimes[8] = {1 + FRAME_LENGTH, 2 + (2*FRAME_LENGTH), 3, 2 + (2*FRAME_LENGTH)};
  size_t nSyncTimes = 4;

  // Test
  bool found = getSystemSyncTime(syncTimes, nSyncTimes, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  assertSyncTimeIsMultipleOfFrameLength(expectedSyncTime, actualSyncTime);
}

void testThatGetSystemSyncTimeDoesNotReturnTimestampFor0Samples()
{
  // Fixture
  uint32_t unused = 0;
  uint32_t syncTimes[8];
  size_t nSyncTimes = 0;

  // Test
  bool found = getSystemSyncTime(syncTimes, nSyncTimes, &unused);

  // Assert
  TEST_ASSERT_FALSE(found);
}

void testThatGetSystemSyncTimeDoesNotReturnTimestampIfTooMuchTimestampsSpread()
{
  // Fixture
  uint32_t unused = 0;
  uint32_t syncTimes[8] = {1, 900, 3};
  size_t nSyncTimes = 3;

  // Test
  bool found = getSystemSyncTime(syncTimes, nSyncTimes, &unused);

  // Assert
  TEST_ASSERT_FALSE(found);
}

void testThatGetSystemSyncTimeHandlesTimestampsWithWrapping()
{
  // Fixture
  uint32_t actualSyncTime = 0;
  uint32_t expectedSyncTime = PULSE_PROCESSOR_TIMESTAMP_MAX;
  uint32_t syncTimes[8] = {0, PULSE_PROCESSOR_TIMESTAMP_MAX - 1, PULSE_PROCESSOR_TIMESTAMP_MAX};
  size_t nSyncTimes = 3;

  // Test
  bool found = getSystemSyncTime(syncTimes, nSyncTimes, &actualSyncTime);

  // Assert
  TEST_ASSERT_TRUE(found);
  TEST_ASSERT_EQUAL(expectedSyncTime, actualSyncTime);
}

bool isSync(pulseProcessor_t *state, unsigned int timestamp);

void testThatIsSyncFindsNextSync0()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + FRAME_LENGTH;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}

void testThatIsSyncFindsNextSync1()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + FRAME_LENGTH + SYNC_SEPARATION;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}

void testThatIsSyncFindsDistantSync1()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + (10*FRAME_LENGTH) + SYNC_SEPARATION;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}

void testThatIsSyncReturnFalseOnSweep()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + FRAME_LENGTH + SWEEP_CENTER;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_FALSE(result);
}

void testThatIsSyncFindsSync0WithSomeNoise()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + FRAME_LENGTH - 10;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}

void testThatIsSyncFindsSync1WithSomeNoise()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + FRAME_LENGTH + SYNC_SEPARATION + 500;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}


void testThatIsSyncFindsSync0WithWrapping()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = PULSE_PROCESSOR_TIMESTAMP_MAX - (FRAME_LENGTH/2);
  uint32_t timestamp = (state.currentSync0 + FRAME_LENGTH) & PULSE_PROCESSOR_TIMESTAMP_MAX;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}

void testThatIsSyncFindsSync1WithWrapping()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = PULSE_PROCESSOR_TIMESTAMP_MAX - (FRAME_LENGTH/2);;
  uint32_t timestamp = (state.currentSync0 + FRAME_LENGTH + SYNC_SEPARATION) & PULSE_PROCESSOR_TIMESTAMP_MAX;;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_TRUE(result);
}

void testThatIsSyncReturnsFalseIfSync1WasSync0AndTheRealSync0IsReceived()
{
  // Fixture
  pulseProcessor_t state = {0};
  state.currentSync0 = 0;
  uint32_t timestamp = state.currentSync0 + FRAME_LENGTH - SYNC_SEPARATION;

  // Test
  bool result = isSync(&state, timestamp);

  // Assert
  TEST_ASSERT_FALSE(result);
}

bool isNewSync(uint32_t timestamp, uint32_t lastSync);

void testThatIsNewSyncMatchesTimestampCloseAfter() {
  // Fixture
  uint32_t lastSync = 4711;
  uint32_t timestamp = lastSync + 3;

  // Test
  bool actual = isNewSync(timestamp, lastSync);

  // Assert
  TEST_ASSERT_FALSE(actual);
}

void testThatIsNewSyncMatchesTimestampCloseBefore() {
  // Fixture
  uint32_t lastSync = 4711;
  uint32_t timestamp = lastSync - 3;

  // Test
  bool actual = isNewSync(timestamp, lastSync);

  // Assert
  TEST_ASSERT_FALSE(actual);
}

void testThatIsNewSyncMatchesTimestampCloseBeforeWhenWrapping() {
  // Fixture
  uint32_t lastSync = 1;
  uint32_t timestamp = (1 << PULSE_PROCESSOR_TIMESTAMP_BITWIDTH) - 1;

  // Test
  bool actual = isNewSync(timestamp, lastSync);

  // Assert
  TEST_ASSERT_FALSE(actual);
}

void testThatIsNewSyncDoesNotMatchTimestampTooFarAway() {
  // Fixture
  uint32_t lastSync = 4711;
  uint32_t timestamp = lastSync + 30;

  // Test
  bool actual = isNewSync(timestamp, lastSync);

  // Assert
  TEST_ASSERT_TRUE(actual);
}


void testThatRsultStructIsCleared() {
  // Fixture
  pulseProcessorResult_t angles;
  angles.sensorMeasurements[2].baseStatonMeasurements[1].validCount = 2;

  // Test
  pulseProcessorClear(&angles, 1);

  // Assert
  TEST_ASSERT_EQUAL_INT(0, angles.sensorMeasurements[2].baseStatonMeasurements[1].validCount);
}

// Test helpers

static void assertSyncTimeIsMultipleOfFrameLength(uint32_t expectedSyncTime, uint32_t actualSyncTime)
{
  uint32_t diff = actualSyncTime - expectedSyncTime;

  TEST_ASSERT_LESS_THAN_MESSAGE(MAX_FRAME_LENGTH_NOISE, diff % FRAME_LENGTH, "Sync time out of bound");
}

static void limitTimestamps(pulseProcessorPulse_t history[])
{
  for (int i=0; i<PULSE_PROCESSOR_HISTORY_LENGTH; i++) {
    history[i].timestamp &= ((1<<PULSE_PROCESSOR_TIMESTAMP_BITWIDTH)-1);
  }
}
