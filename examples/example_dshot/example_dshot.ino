#include "dshot.h"

constexpr int pins[] = { 3, 4, 5, 6 };
constexpr unsigned int pin_number = sizeof(pins) / sizeof(pins[0]);

void setup() {
  Serial.begin(9600);
  dshotFrequency(DShot300);

  // Initialize and check all pins and check the pin works
  for (unsigned int i = 0; i < pin_number; i++) {
    Serial.print("Pin Number ");
    Serial.print(pins[i]);
    Serial.print(" : ");
    switch (sendDSHOT(pins[0], 0, false)) {
      case DShotError_Success:
        Serial.println("SUCCESS!");
        break;
      case DShotError_NotAPin:
        Serial.println("FAILED -> This is not a pin!");
        break;
      case DShotError_NotFlexPWM:
        Serial.println("FAILED -> This pin is not a FlexPWM pin!");
        break;
      case DShotError_SubmoduleUsed:
        Serial.println("FAILED -> Submodule already in use!");
        break;
      case DShotError_NoSource:
        Serial.println("FAILED -> Unknown DMA Source!");
        break;
      case DShotError_NoDMAAvailable:
        Serial.println("FAILED -> No more DMA channels available!");
        break;
      default:
        Serial.println("FAILED -> Unknown error!");
    }
  }

  // Arm the ESC by sending the arm command (0)
  for (int j = 0; j < 20; j++) {
    for (unsigned int i = 0; i < pin_number; i++) {
      sendDSHOT(pins[i], 0, false);
    }
    delay(50);
  }

  while (Serial.available()) Serial.read(); // Wait for a message
}

void loop() {
  static uint16_t throttle = 47;
  static uint16_t throttle_target = 47;

  // Read the throttle
  if (Serial.available()) {
    throttle_target = constrain((uint16_t)Serial.parseInt(), 0, DSHOT_THROTTLE_MAX);
    while (Serial.available()) Serial.read();
  }

  /*// Move the throttle towards the target
  if (throttle < throttle_target) throttle++;
  if (throttle > throttle_target) throttle--;
  */
  throttle = throttle_target;

  // Send new throttles
  for (unsigned int i = 0; i < pin_number; i++) {
    sendDSHOT(pins[i], throttle, false);
  }

  // Print target and actual throttles
  Serial.print(throttle_target);
  Serial.print(" | ");
  Serial.println(throttle);

  // Slow the processor down
  delay(25);
}
