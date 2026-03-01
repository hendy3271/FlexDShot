#include "dshot.h"
#include "DMAChannel.h"

#ifndef __IMXRT1062__
#error THIS MODULE ONLY WORKS WITH TEENSY 4.0 AND 4.1
#endif

void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, float frequency);

template<uint8_t submodule_number> static void ISR();

// Direct from pwm.c

#include "imxrt.h"
#include "core_pins.h"

struct pwm_pin_info_struct {
  uint8_t type;     // 0=no pwm, 1=flexpwm, 2=quad
  uint8_t flex_module;   // 0-3, 0-3
  uint8_t channel;  // 0=X, 1=A, 2=B
  uint8_t muxval;   //
};

#define M(a, b) (1 << (b + ((a - 1) << 2)))  // FM4S3 FM4S2 FM4S1 FM4S0 FM3S3 FM3S2 FM3S1 FM3S0 FM2S3 FM2S2 FM2S1 FM2S0 FM1S3 FM1S2 FM1S1 FM1S0
#define _M(a, b) (b + ((a - 1) << 2))  // Bit position of module

#define SM(m) (__builtin_ctz(m) & 0x3)
#define _SM(m) (m & 0x3)
#define FM(m) ((__builtin_ctz(m) >> 2) + 1)
#define _FM(m) ((m >> 2) + 1)

#define FLEXPWM_SUBMODULE_NUMBER 16

#if defined(__IMXRT1062__)

const struct pwm_pin_info_struct pwm_pin_info[] = {
  { 1, _M(1, 1), 0, 4 },  // FlexPWM1_1_X   0  // AD_B0_03
  { 1, _M(1, 0), 0, 4 },  // FlexPWM1_0_X   1  // AD_B0_02
  { 1, _M(4, 2), 1, 1 },  // FlexPWM4_2_A   2  // EMC_04
  { 1, _M(4, 2), 2, 1 },  // FlexPWM4_2_B   3  // EMC_05
  { 1, _M(2, 0), 1, 1 },  // FlexPWM2_0_A   4  // EMC_06
  { 1, _M(2, 1), 1, 1 },  // FlexPWM2_1_A   5  // EMC_08
  { 1, _M(2, 2), 1, 2 },  // FlexPWM2_2_A   6  // B0_10
  { 1, _M(1, 3), 2, 6 },  // FlexPWM1_3_B   7  // B1_01
  { 1, _M(1, 3), 1, 6 },  // FlexPWM1_3_A   8  // B1_00
  { 1, _M(2, 2), 2, 2 },  // FlexPWM2_2_B   9  // B0_11
  { 2, _M(1, 0), 0, 1 },  // QuadTimer1_0  10  // B0_00
  { 2, _M(1, 2), 0, 1 },  // QuadTimer1_2  11  // B0_02
  { 2, _M(1, 1), 0, 1 },  // QuadTimer1_1  12  // B0_01
  { 2, _M(2, 0), 0, 1 },  // QuadTimer2_0  13  // B0_03
  { 2, _M(3, 2), 0, 1 },  // QuadTimer3_2  14  // AD_B1_02
  { 2, _M(3, 3), 0, 1 },  // QuadTimer3_3  15  // AD_B1_03
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 2, _M(3, 1), 0, 1 },  // QuadTimer3_1  18  // AD_B1_01
  { 2, _M(3, 0), 0, 1 },  // QuadTimer3_0  19  // AD_B1_00
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 1, _M(4, 0), 1, 1 },  // FlexPWM4_0_A  22  // AD_B1_08
  { 1, _M(4, 1), 1, 1 },  // FlexPWM4_1_A  23  // AD_B1_09
  { 1, _M(1, 2), 0, 4 },  // FlexPWM1_2_X  24  // AD_B0_12
  { 1, _M(1, 3), 0, 4 },  // FlexPWM1_3_X  25  // AD_B0_13
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 1, _M(3, 1), 2, 1 },  // FlexPWM3_1_B  28  // EMC_32
  { 1, _M(3, 1), 1, 1 },  // FlexPWM3_1_A  29  // EMC_31
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 1, _M(2, 0), 2, 1 },  // FlexPWM2_0_B  33  // EMC_07 //17
#ifdef ARDUINO_TEENSY40
  { 1, _M(1, 1), 2, 1 },  // FlexPWM1_1_B  34  // SD_B0_03
  { 1, _M(1, 1), 1, 1 },  // FlexPWM1_1_A  35  // SD_B0_02
  { 1, _M(1, 0), 2, 1 },  // FlexPWM1_0_B  36  // SD_B0_01
  { 1, _M(1, 0), 1, 1 },  // FlexPWM1_0_A  37  // SD_B0_00
  { 1, _M(1, 2), 2, 1 },  // FlexPWM1_2_B  38  // SD_B0_05
  { 1, _M(1, 2), 1, 1 },  // FlexPWM1_2_A  39  // SD_B0_04 //6
#endif
#ifdef ARDUINO_TEENSY41
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 1, _M(2, 3), 1, 6 },  // FlexPWM2_3_A  36  // B1_00
  { 1, _M(2, 3), 2, 6 },  // FlexPWM2_3_B  37  // B1_01
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 0, _M(1, 0), 0, 0 },
  { 1, _M(1, 1), 2, 1 },  // FlexPWM1_1_B  42  // SD_B0_03
  { 1, _M(1, 1), 1, 1 },  // FlexPWM1_1_A  43  // SD_B0_02
  { 1, _M(1, 0), 2, 1 },  // FlexPWM1_0_B  44  // SD_B0_01
  { 1, _M(1, 0), 1, 1 },  // FlexPWM1_0_A  45  // SD_B0_00
  { 1, _M(1, 2), 2, 1 },  // FlexPWM1_2_B  46  // SD_B0_05
  { 1, _M(1, 2), 1, 1 },  // FlexPWM1_2_A  47  // SD_B0_04
  { 0, _M(1, 0), 0, 0 },  // duplicate FlexPWM1_0_B
  { 0, _M(1, 0), 0, 0 },  // duplicate FlexPWM1_2_A
  { 0, _M(1, 0), 0, 0 },  // duplicate FlexPWM1_2_B
  { 1, _M(3, 3), 2, 1 },  // FlexPWM3_3_B  51  // EMC_22
  { 0, _M(1, 0), 0, 0 },  // duplicate FlexPWM1_1_B
  { 0, _M(1, 0), 0, 0 },  // duplicate FlexPWM1_1_A
  { 1, _M(3, 0), 1, 1 },  // FlexPWM3_0_A  54  // EMC_29 //10
#endif
#ifdef ARDUINO_TEENSY_MICROMOD
  { 1, _M(1, 1), 2, 1 },  // FlexPWM1_1_B  34  // SD_B0_03
  { 1, _M(1, 1), 1, 1 },  // FlexPWM1_1_A  35  // SD_B0_02
  { 1, _M(1, 0), 2, 1 },  // FlexPWM1_0_B  36  // SD_B0_01
  { 1, _M(1, 0), 1, 1 },  // FlexPWM1_0_A  37  // SD_B0_00
  { 1, _M(1, 2), 1, 1 },  // FlexPWM1_2_A  38  // SD_B0_04
  { 1, _M(1, 2), 2, 1 },  // FlexPWM1_2_B  39  // SD_B0_05
  { 2, _M(2, 1), 0, 1 },  // QuadTimer2_1  40  // B0_04
  { 2, _M(2, 2), 0, 1 },  // QuadTimer2_2  41  // B0_05
  { 0, _M(1, 0), 0, 0 },  // duplicate QuadTimer3_0
  { 0, _M(1, 0), 0, 0 },  // duplicate QuadTimer3_1
  { 0, _M(1, 0), 0, 0 },  // duplicate QuadTimer3_2
  { 2, _M(4, 0), 0, 1 },  // QuadTimer4_0  45  // B0_09
#endif
};

// Known usage of FlexPWM and QuadTimers
// -------------------------------------
//   FlexPWM1_0    PWM pin 1, 36(T4.0), 37(T4.0), 44(T4.1), 45(T4.1)
//   FlexPWM1_1    PWM pin 0, 34(T4.0), 35(T4.0), 42(T4.1), 43(T4.1)
//   FlexPWM1_2    PWM pin 24, 38(T4.0), 39(T4.0), 46(T4.1), 47(T4.1)
//   FlexPWM1_3    PWM pin 7, 8, 25
//   FlexPWM2_0    PWM pin 4, 33
//   FlexPWM2_1    PWM pin 5, Tlc5940 library
//   FlexPWM2_2    PWM pin 6, 9
//   FlexPWM2_3    PWM pin 36(T4.1), 37(T4.1)
//   FlexPWM3_0    PWM pin 53(T4.1)
//   FlexPWM3_1    PWM pin 28, 29
//   FlexPWM3_2
//   FlexPWM3_3    PWM pin 51(T4.1)
//   FlexPWM4_0    PWM pin 22
//   FlexPWM4_1    PWM pin 23
//   FlexPWM4_2    PWM pin 2, 3, Tlc5940 library
//   FlexPWM4_3
//   QuadTimer1_0  PWM pin 10
//   QuadTimer1_1  PWM pin 12
//   QuadTimer1_2  PWM pin 11
//   QuadTimer1_3
//   QuadTimer2_0  PWM pin 13
//   QuadTimer2_1
//   QuadTimer2_2
//   QuadTimer2_3
//   QuadTimer3_0  PWM pin 19
//   QuadTimer3_1  PWM pin 18
//   QuadTimer3_2  PWM pin 14
//   QuadTimer3_3  PWM pin 15
//   QuadTimer4_0  OctoWS2811, ADC library
//   QuadTimer4_1  OctoWS2811
//   QuadTimer4_2  OctoWS2811
//   QuadTimer4_3  AudioInputAnalog, ADC library


#endif  // __IMXRT1062__

// End direct copy of pwm.c

void (*dma_isr[FLEXPWM_SUBMODULE_NUMBER])() = { ISR<0>, ISR<1>, ISR<2>, ISR<3>, ISR<4>, ISR<5>, ISR<6>, ISR<7>, ISR<8>, ISR<9>,
                                        ISR<10>, ISR<11>, ISR<12>, ISR<13>, ISR<14>, ISR<15> };

const uint16_t dshotFreq[DShotType_Count] = { 150, 300, 600, 1200 };

const uint16_t DATA_LENGTH = 20;

struct flexpwm_buffer {
    uint16_t VAL0 = 0;      // Mid-cycle reload (if used)
    uint16_t VAL1 = 0;      // Period
    uint16_t FRACVAL2 = 0;  // PWMA fractional
    uint16_t VAL2 = 0;      // PWMA rising edge
    uint16_t FRACVAL3 = 0;
    uint16_t VAL3 = 0;      // PWMA falling edge
    uint16_t FRACVAL4 = 0;
    uint16_t VAL4 = 0;      // PWMB rising
    uint16_t FRACVAL5 = 0;
    uint16_t VAL5 = 0;      // PWMB falling
    uint16_t FRACVAL1 = 0;  // If needed
};

struct flexpwm_submodule_pins {
  uint8_t pinX = 255;  // X
  uint8_t pinA = 255;  // A
  uint8_t pinB = 255;  // B
};

DMAChannel *dma_channels[FLEXPWM_SUBMODULE_NUMBER] = { nullptr };
flexpwm_buffer dma_data[FLEXPWM_SUBMODULE_NUMBER][DATA_LENGTH] = {};
flexpwm_submodule_pins submodule_pins[FLEXPWM_SUBMODULE_NUMBER] = {};

float dshot_freq = dshotFreq[DShot150];
uint64_t dma_pins = 0;            // "Bit array" for pin tracking
uint16_t flexpwm_submodules = 0;  // "Bit array" for submodule tracking M(flex_module, submodule)

uint16_t dshotFrequency() {
  //Serial.println("dshotFrequency()");
  return dshot_freq;
}

uint16_t dshotFrequency(uint8_t dshot_speed) {
  //Serial.println("dshotFrequency()");
  if (dshot_speed >= DShotType_Count) return DShotType_Invalid;  // Return an error if there is not compatable mode
  if (dma_pins > 0) return DShotType_Error;                      // Return and error if any pin is initialized TODO : uninitialization
  dshot_freq = dshotFreq[dshot_speed];
  return dshot_freq;
}

// Inspired by pwm.c : void analogWriteFrequency(uint8_t pin, float frequency)
int dshotInit(uint8_t pin)  // Returns whether the pin is DShot capable (ie FlexPWM)
{
  //Serial.println("dshotInit()");
  const struct pwm_pin_info_struct *info;

  if (pin >= CORE_NUM_DIGITAL) return DShotError_NotAPin;  // Not even a pin!

  //Serial.printf("analogWriteFrequency, pin %d, freq %d\n", pin, (int)frequency);
  info = pwm_pin_info[pin];

  if (info->type == 1) {
    // FlexPWM pin
    
    unsigned int submodule = _SM(info->flex_module);
    unsigned int flex_module = _FM(info->flex_module);
    uint16_t mask = 1 << submodule;

    IMXRT_FLEXPWM_t *flexpwm = nullptr;

    if ((flexpwm_submodules & M(flex_module, submodule)) == 0) { 
      // Submodule not yet used, need to configure
      flexpwm_submodules |= M(flex_module, submodule);

      if constexpr (flex_module == 1) flexpwm = &IMXRT_FLEXPWM1;
      else if constexpr (flex_module == 2) flexpwm = &IMXRT_FLEXPWM2;
      else if constexpr (flex_module == 3) flexpwm = &IMXRT_FLEXPWM3;
      else if constexpr (flex_module == 4) flexpwm = &IMXRT_FLEXPWM4;
      else return DShotError_Error;  // Invalid module, should never happen

      flexpwmFrequency(flexpwm, submodule, info->channel, dshot_freq);

      // Configure DMA
      unsigned int dma_source = 255;
      switch (info->flex_module) {
        case M(1, 0):
          dma_source = DMAMUX_SOURCE_FLEXPWM1_WRITE0;
          break;
        case M(1, 1):
          dma_source = DMAMUX_SOURCE_FLEXPWM1_WRITE1;
          break;
        case M(1, 2):
          dma_source = DMAMUX_SOURCE_FLEXPWM1_WRITE2;
          break;
        case M(1, 3):
          dma_source = DMAMUX_SOURCE_FLEXPWM1_WRITE3;
          break;
        case M(2, 0):
          dma_source = DMAMUX_SOURCE_FLEXPWM2_WRITE0;
          break;
        case M(2, 1):
          dma_source = DMAMUX_SOURCE_FLEXPWM2_WRITE1;
          break;
        case M(2, 2):
          dma_source = DMAMUX_SOURCE_FLEXPWM2_WRITE2;
          break;
        case M(2, 3):
          dma_source = DMAMUX_SOURCE_FLEXPWM2_WRITE3;
          break;
        case M(3, 0):
          dma_source = DMAMUX_SOURCE_FLEXPWM3_WRITE0;
          break;
        case M(3, 1):
          dma_source = DMAMUX_SOURCE_FLEXPWM3_WRITE1;
          break;
        case M(3, 2):
          dma_source = DMAMUX_SOURCE_FLEXPWM3_WRITE2;
          break;
        case M(3, 3):
          dma_source = DMAMUX_SOURCE_FLEXPWM3_WRITE3;
          break;
        case M(4, 0):
          dma_source = DMAMUX_SOURCE_FLEXPWM4_WRITE0;
          break;
        case M(4, 1):
          dma_source = DMAMUX_SOURCE_FLEXPWM4_WRITE1;
          break;
        case M(4, 2):
          dma_source = DMAMUX_SOURCE_FLEXPWM4_WRITE2;
          break;
        case M(4, 3):
          dma_source = DMAMUX_SOURCE_FLEXPWM4_WRITE3;
          break;
      }
      if (dma_source == 255) return DShotError_NoSource;  // Doesn't match a known DMA source from FlexPWM

      dma_channels[submodule] = new DMAChannel();

      if (!dma_channels[submodule]) return DShotError_NoDMAAvailable;  // Your out of DMA Channels

      dma_channels[submodule]->sourceBuffer(dma_data[submodule], sizeof(dma_data[submodule]));
      dma_channels[submodule]->destination((uint16_t &)flexpwm->SM[submodule].VAL0); 
      dma_channels[submodule]->triggerAtHardwareEvent(dma_source);
      dma_channels[submodule]->interruptAtCompletion();
      dma_channels[submodule]->attachInterrupt(dma_isr[submodule]);
      dma_channels[submodule]->enable();
      
    }

    if (flexpwm == nullptr) {
      return DShotError_Error;  // Checker, should never happen
    }

    // Channel configuration and pin tracking
    if(info->channel == 0) {
      if(submodule_pins[submodule].pinX != 255) return DShotError_ChannelUsed;  // Channel X already used by another pin
      else {
        submodule_pins[submodule].pinX = pin;
        flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMX_EN(mask);
      }
    } else if(info->channel == 1)
      if(submodule_pins[submodule].pinA != 255) return DShotError_ChannelUsed;  // Channel A already used by another pin
      else {
        submodule_pins[submodule].pinA = pin;
        flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMA_EN(mask);
      }
    } else if(info->channel == 2) {
      if(submodule_pins[submodule].pinB != 255) return DShotError_ChannelUsed;  // Channel B already used by another pin
      else {
        submodule_pins[submodule].pinB = pin;
        flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMB_EN(mask);
      }
    }

    pinMode(pin, OUTPUT);
    *(portConfigRegister(pin)) = info->muxval;

    return DShotError_Success;  // DShot capable
  }
  return DShotError_NotFlexPWM;  // Not DShot capable (QuadTimer)
}

// Inspired by pwm.c : flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel __attribute__((unused)), float frequency)
void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, float frequency) {
  //Serial.println("flexpwmFrequency()");
  uint16_t mask = 1 << submodule;
  uint32_t newdiv = (uint32_t)((float)F_BUS_ACTUAL / frequency + 0.5f);

  //Serial.printf(" div=%lu, scale=%lu\n", newdiv, 1);
  p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
  p->SM[submodule].CTRL = FLEXPWM_SMCTRL_FULL;  // | FLEXPWM_SMCTRL_PRSC(7);
  p->SM[submodule].VAL0 = 0;  // X module
  p->SM[submodule].VAL1 = newdiv - 1;
  p->SM[submodule].VAL2 = 0;  // For edge aligned
  p->SM[submodule].VAL3 = 0;  // A module
  p->SM[submodule].VAL4 = 0;  // For edge aligned
  p->SM[submodule].VAL5 = 0;  // B module
  p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
  p->SM[submodule].DMAEN = FLEXPWM_SMDMAEN_VALDE;

  for (int i = 0; i < DATA_LENGTH; i++) {
    dma_data[submodule][i].VAL0 = newdiv - 1;
    dma_data[submodule][i].VAL1 = newdiv - 1;
  }
}


// Inspired by pwm.c : void analogWrite(uint8_t pin, int val)
int sendDSHOT(uint8_t pin, uint16_t throttle, bool telemetry) {
  //Serial.println("sendDSHOT()");
  if (pin >= CORE_NUM_DIGITAL) return DShotError_NotAPin;

  if (((dma_pins >> pin) & 1) == 0) {
    int error = dshotInit(pin);
    if (error > 0) return error;
    dma_pins |= 1 << pin;
  }

  //Serial.printf("sendDSHOT, pin %d, throttle %d, telemetry %c\n", pin, throttle, telemetry ? 'T' : 'F');

  const struct pwm_pin_info_struct *info;
  info = pwm_pin_info[pin];
  
  unsigned int submodule = _SM(info->flex_module);
  unsigned int flex_module = _FM(info->flex_module);
  unsigned int channel = info->channel;

  if (info->type == 1) {
    // FlexPWM pin
    uint16_t data = (constrain(throttle, 0, DSHOT_THROTTLE_MAX) << 5) | (telemetry << 4);
    data |= ((data >> 4) ^ (data >> 8) ^ (data >> 12)) & 0x0f;

    IMXRT_FLEXPWM_t *flexpwm;
    
    if constexpr (flex_module == 1) flexpwm = &IMXRT_FLEXPWM1;
    else if constexpr (flex_module == 2) flexpwm = &IMXRT_FLEXPWM2;
    else if constexpr (flex_module == 3) flexpwm = &IMXRT_FLEXPWM3;
    else if constexpr (flex_module == 4) flexpwm = &IMXRT_FLEXPWM4;
    else return DShotError_Error;  // Invalid module, should never happen

    uint32_t bit_length = flexpwm->SM[submodule].VAL1;

    if (info->channel == 0) {  // X : inverted need to zero because we will flip later.
      for (int i = 0; i < 16; i++) {
        uint32_t offset = (data >> i) & 1;
        dma_data[submodule][i].VAL0 = bit_length - (bit_length * 3) >> (3 - offset);
      }
    } else if (info->channel == 1) {  // A : rising edge
      for (int i = 0; i < 16; i++) {
        uint32_t offset = (data >> i) & 1;
        dma_data[submodule][i].VAL2 = (bit_length * 3) >> (3 - offset);
      }
    } else if (info->channel == 2) {  // B : rising edge
      for (int i = 0; i < 16; i++) {
        uint32_t offset = (data >> i) & 1;
        dma_data[submodule][i].VAL4 = (bit_length * 3) >> (3 - offset);
      }
    }

    flexpwm->MCTRL |= FLEXPWM_MCTRL_RUN(1 << submodule);

    return DShotError_Success;
  }
  return DShotError_NotFlexPWM;
}

template<uint8_t submodule_number>
static void ISR() {
  //Serial.print("ISR() : ");
  //Serial.println(pin);

  const struct pwm_pin_info_struct *info;
  info = pwm_pin_info[pin];

  constexpr unsigned int submodule = _SM(submodule_number);
  constexpr unsigned int flex_module = _FM(submodule_number);

  IMXRT_FLEXPWM_t *flexpwm;
  
  if constexpr (flex_module == 1) flexpwm = &IMXRT_FLEXPWM1;
  else if constexpr (flex_module == 2) flexpwm = &IMXRT_FLEXPWM2;
  else if constexpr (flex_module == 3) flexpwm = &IMXRT_FLEXPWM3;
  else if constexpr (flex_module == 4) flexpwm = &IMXRT_FLEXPWM4;
  else return;  // Invalid module, should never happen

  flexpwm->MCTRL &= FLEXPWM_MCTRL_RUN(~(1 << submodule));
  dma_channels[submodule]->clearInterrupt();
}