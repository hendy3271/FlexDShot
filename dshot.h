#include <Arduino.h>

enum DShotType {
  DShot150,
  DShot300,
  DShot600,
  DShot1200,
  DShotType_Count,
};

enum DShotError {
  DShotError_Success,
  DShotError_NotAPin,
  DShotError_NotFlexPWM,
  DShotError_SubmoduleUsed,
  DShotError_NoSource,
  DShotError_NoDMAAvailable,
  DShotError_Count
};

const uint16_t DSHOT_THROTTLE_MAX = 0x7FF;  // or (1 << 11) - 1; // Max value of 11-bit unsigned integer

float dshotFrequency();                     // Returns current frequency in bits/s
float dshotFrequency(uint8_t dshot_speed);  // Defaults to DShot150 (cannot update once any message is sent)

int sendDSHOT(uint8_t pin, uint16_t throttle = 0, bool telemetry = false);  // Returns DShotError (see enum)