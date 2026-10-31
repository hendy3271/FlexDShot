#include "dshot.h"
#include "DMAChannel.h"

#ifndef __IMXRT1062__
#error THIS MODULE ONLY WORKS WITH TEENSY 4.0, 4.1 AND MICROMOD
#endif

static void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, unsigned int sm_index, float frequency);

template<uint8_t submodule_number> static void ISR();

// Direct from pwm.c

#include "imxrt.h"
#include "core_pins.h"

struct pwm_pin_info_struct {
  uint8_t type;     // 0=no pwm, 1=flexpwm, 2=quad
  uint8_t flex_module;   // Submodule index _M(module, submodule): 0-15
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

static const struct pwm_pin_info_struct pwm_pin_info[] = {
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

static void (*const dma_isr[FLEXPWM_SUBMODULE_NUMBER])() = { ISR<0>, ISR<1>, ISR<2>, ISR<3>, ISR<4>, ISR<5>, ISR<6>, ISR<7>,
                                                             ISR<8>, ISR<9>, ISR<10>, ISR<11>, ISR<12>, ISR<13>, ISR<14>, ISR<15> };

static const float dshotFreq[DShotType_Count] = { 150'000, 300'000, 600'000, 1'200'000 };  // bits/s

static const uint16_t DATA_LENGTH = 20;              // 2 idle bits + 16 data bits + 2 idle bits
static const uint32_t BUSY_TIMEOUT_US = 1000;        // Longest frame (DShot150, 20 bits) is ~133us

// One DMA "minor loop" per PWM period: rewrites VAL0..VAL5 of a submodule in register order
// (must match IMXRT_FLEXPWM_t::SM[] layout starting at VAL0).
struct flexpwm_buffer {
  uint16_t VAL0 = 0;      // PWMX turn-on (X is high from VAL0 to VAL1)
  uint16_t FRACVAL1 = 0;
  uint16_t VAL1 = 0;      // Period
  uint16_t FRACVAL2 = 0;
  uint16_t VAL2 = 0;      // PWMA rising edge
  uint16_t FRACVAL3 = 0;
  uint16_t VAL3 = 0;      // PWMA falling edge
  uint16_t FRACVAL4 = 0;
  uint16_t VAL4 = 0;      // PWMB rising edge
  uint16_t FRACVAL5 = 0;
  uint16_t VAL5 = 0;      // PWMB falling edge
};
static_assert(sizeof(flexpwm_buffer) == 22, "flexpwm_buffer must mirror VAL0..VAL5 registers");

struct flexpwm_submodule_pins {
  uint8_t pinX = 255;  // X
  uint8_t pinA = 255;  // A
  uint8_t pinB = 255;  // B
};

// All per-submodule arrays are indexed by _M(module, submodule) (0-15)
static DMAChannel *dma_channels[FLEXPWM_SUBMODULE_NUMBER] = { nullptr };
static flexpwm_buffer dma_data[FLEXPWM_SUBMODULE_NUMBER][DATA_LENGTH] = {};
static flexpwm_submodule_pins submodule_pins[FLEXPWM_SUBMODULE_NUMBER] = {};

static float dshot_freq = dshotFreq[DShot150];
static uint64_t dma_pins = 0;                  // "Bit array" for pin tracking (1ULL << pin)
static uint16_t flexpwm_submodules = 0;        // "Bit array" for submodule tracking M(flex_module, submodule)
static volatile uint16_t busy_submodules = 0;  // "Bit array" of submodules with a frame in flight (cleared in ISR)

static const uint8_t dma_sources[FLEXPWM_SUBMODULE_NUMBER] = {
  DMAMUX_SOURCE_FLEXPWM1_WRITE0, DMAMUX_SOURCE_FLEXPWM1_WRITE1, DMAMUX_SOURCE_FLEXPWM1_WRITE2, DMAMUX_SOURCE_FLEXPWM1_WRITE3,
  DMAMUX_SOURCE_FLEXPWM2_WRITE0, DMAMUX_SOURCE_FLEXPWM2_WRITE1, DMAMUX_SOURCE_FLEXPWM2_WRITE2, DMAMUX_SOURCE_FLEXPWM2_WRITE3,
  DMAMUX_SOURCE_FLEXPWM3_WRITE0, DMAMUX_SOURCE_FLEXPWM3_WRITE1, DMAMUX_SOURCE_FLEXPWM3_WRITE2, DMAMUX_SOURCE_FLEXPWM3_WRITE3,
  DMAMUX_SOURCE_FLEXPWM4_WRITE0, DMAMUX_SOURCE_FLEXPWM4_WRITE1, DMAMUX_SOURCE_FLEXPWM4_WRITE2, DMAMUX_SOURCE_FLEXPWM4_WRITE3,
};

static inline IMXRT_FLEXPWM_t *flexpwmModule(unsigned int flex_module) {  // flex_module is 1-4
  return flex_module == 1 ? &IMXRT_FLEXPWM1 : flex_module == 2 ? &IMXRT_FLEXPWM2
                                            : flex_module == 3 ? &IMXRT_FLEXPWM3
                                                               : &IMXRT_FLEXPWM4;
}

float dshotFrequency() {
  return dshot_freq;
}

float dshotFrequency(uint8_t dshot_speed) {
  if (dshot_speed >= DShotType_Count) return -1.0f;  // Not a DShot speed
  if (dma_pins > 0) return -2.0f;                    // A pin is already initialized TODO : uninitialization
  dshot_freq = dshotFreq[dshot_speed];
  return dshot_freq;
}

// Point the DMA at the start of the submodule's buffer: one 22-byte minor loop (VAL0..VAL5) per PWM reload,
// destination rewinds by 22 bytes after each minor loop, DATA_LENGTH minor loops per frame.
static void dmaReset(unsigned int sm_index) {
  const unsigned int submodule = _SM(sm_index);
  IMXRT_FLEXPWM_t *flexpwm = flexpwmModule(_FM(sm_index));
  volatile auto *tcd = dma_channels[sm_index]->TCD;
  const int32_t minor = sizeof(flexpwm_buffer);

  tcd->SADDR = dma_data[sm_index];
  tcd->SOFF = 2;
  tcd->ATTR = DMA_TCD_ATTR_SSIZE(1) | DMA_TCD_ATTR_DSIZE(1);
  tcd->NBYTES_MLOFFYES = DMA_TCD_NBYTES_DMLOE | DMA_TCD_NBYTES_MLOFFYES_MLOFF(-minor) | DMA_TCD_NBYTES_MLOFFYES_NBYTES(minor);
  tcd->SLAST = -(int32_t)sizeof(dma_data[sm_index]);
  tcd->DADDR = &flexpwm->SM[submodule].VAL0;
  tcd->DOFF = 2;
  tcd->DLASTSGA = 0;
  tcd->CITER = DATA_LENGTH;
  tcd->BITER = DATA_LENGTH;
}

// Inspired by pwm.c : void analogWriteFrequency(uint8_t pin, float frequency)
static int dshotInit(uint8_t pin)  // Returns whether the pin is DShot capable (ie FlexPWM)
{
  if (pin >= CORE_NUM_DIGITAL) return DShotError_NotAPin;  // Not even a pin!

  const struct pwm_pin_info_struct *info = &pwm_pin_info[pin];
  if (info->type != 1) return DShotError_NotFlexPWM;  // Not DShot capable (QuadTimer / no PWM)

  const unsigned int sm_index = info->flex_module;
  const unsigned int submodule = _SM(sm_index);
  const unsigned int flex_module = _FM(sm_index);
  const uint16_t mask = 1 << submodule;
  IMXRT_FLEXPWM_t *flexpwm = flexpwmModule(flex_module);

  // Check the channel first so a failure leaves nothing half-configured
  uint8_t *channel_pin = info->channel == 0 ? &submodule_pins[sm_index].pinX
                       : info->channel == 1 ? &submodule_pins[sm_index].pinA
                                            : &submodule_pins[sm_index].pinB;
  if (*channel_pin != 255) return DShotError_ChannelUsed;  // Channel already used by another pin

  if ((flexpwm_submodules & M(flex_module, submodule)) == 0) {
    // Submodule not yet used, need to configure (one DMA channel per submodule, shared by X/A/B)
    if (sm_index >= FLEXPWM_SUBMODULE_NUMBER) return DShotError_NoSource;  // Doesn't match a known DMA source

    DMAChannel *dma = new DMAChannel();
    if (dma->TCD == nullptr) {  // DMAChannel couldn't allocate a hardware channel
      delete dma;               // Nothing claimed yet, so a later retry starts clean
      return DShotError_NoDMAAvailable;
    }
    dma_channels[sm_index] = dma;

    flexpwmFrequency(flexpwm, submodule, sm_index, dshot_freq);

    dmaReset(sm_index);
    dma->triggerAtHardwareEvent(dma_sources[sm_index]);
    dma->interruptAtCompletion();
    dma->attachInterrupt(dma_isr[sm_index]);
    dma->enable();

    flexpwm_submodules |= M(flex_module, submodule);  // Only claim once setup has fully succeeded
  }

  // Channel configuration and pin tracking
  *channel_pin = pin;
  switch (info->channel) {
    case 0: flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMX_EN(mask); break;
    case 1: flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMA_EN(mask); break;
    case 2: flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMB_EN(mask); break;
  }

  pinMode(pin, OUTPUT);
  *(portConfigRegister(pin)) = info->muxval;

  return DShotError_Success;  // DShot capable
}

// Inspired by pwm.c : flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel __attribute__((unused)), float frequency)
static void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, unsigned int sm_index, float frequency) {
  uint16_t mask = 1 << submodule;
  uint32_t newdiv = (uint32_t)((float)F_BUS_ACTUAL / frequency + 0.5f);

  p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
  p->SM[submodule].CTRL = FLEXPWM_SMCTRL_FULL;  // | FLEXPWM_SMCTRL_PRSC(7);
  p->SM[submodule].VAL0 = newdiv - 1;  // X module (high from VAL0 to VAL1, so VAL0 = VAL1 is low)
  p->SM[submodule].VAL1 = newdiv - 1;
  p->SM[submodule].VAL2 = 0;  // A rising edge (edge aligned)
  p->SM[submodule].VAL3 = 0;  // A falling edge
  p->SM[submodule].VAL4 = 0;  // B rising edge (edge aligned)
  p->SM[submodule].VAL5 = 0;  // B falling edge
  p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
  p->SM[submodule].DMAEN = FLEXPWM_SMDMAEN_VALDE;

  // Idle (low) values for every bit slot and every channel
  for (int i = 0; i < DATA_LENGTH; i++) {
    dma_data[sm_index][i] = flexpwm_buffer();
    dma_data[sm_index][i].VAL0 = newdiv - 1;
    dma_data[sm_index][i].VAL1 = newdiv - 1;
  }
}


// Inspired by pwm.c : void analogWrite(uint8_t pin, int val)
int sendDSHOT(uint8_t pin, uint16_t throttle, bool telemetry) {
  if (pin >= CORE_NUM_DIGITAL) return DShotError_NotAPin;

  if (((dma_pins >> pin) & 1) == 0) {
    int error = dshotInit(pin);
    if (error > 0) return error;
    dma_pins |= 1ULL << pin;  // 64-bit shift: pins >= 32 exist on Teensy 4.1 / MicroMod
  }

  const struct pwm_pin_info_struct *info = &pwm_pin_info[pin];
  if (info->type != 1) return DShotError_NotFlexPWM;

  const unsigned int sm_index = info->flex_module;
  const unsigned int submodule = _SM(sm_index);
  IMXRT_FLEXPWM_t *flexpwm = flexpwmModule(_FM(sm_index));
  const uint16_t sm_bit = 1 << sm_index;

  uint16_t data = (constrain(throttle, 0, DSHOT_THROTTLE_MAX) << 5) | (telemetry << 4);
  data |= ((data >> 4) ^ (data >> 8) ^ (data >> 12)) & 0x0f;

  // Wait for any frame in flight on this submodule (it may belong to another pin sharing it)
  elapsedMicros waited;
  while ((busy_submodules & sm_bit) && waited < BUSY_TIMEOUT_US) {}
  if (busy_submodules & sm_bit) {  // ISR never fired: stop the submodule and recover
    flexpwm->MCTRL &= ~FLEXPWM_MCTRL_RUN(1 << submodule);
    dma_channels[sm_index]->clearComplete();
    busy_submodules &= ~sm_bit;
  }

  const uint32_t bit_length = flexpwm->SM[submodule].VAL1;

  // MSB first, in slots 2..17. High time: 3/8 (37.5%) of a bit for a 0, 3/4 (75%) for a 1.
  for (int i = 0; i < 16; i++) {
    const uint32_t offset = (data >> i) & 1;
    const uint16_t high = (bit_length * 3) >> (3 - offset);
    flexpwm_buffer &slot = dma_data[sm_index][DATA_LENGTH - 3 - i];
    switch (info->channel) {
      case 0: slot.VAL0 = bit_length - high; break;  // X : high from VAL0 to VAL1 (inverted)
      case 1: slot.VAL2 = 0; slot.VAL3 = high; break; // A : rise at VAL2, fall at VAL3
      case 2: slot.VAL4 = 0; slot.VAL5 = high; break; // B : rise at VAL4, fall at VAL5
    }
  }

  // Note: other pins on the same submodule resend their last frame alongside this one.

  // Restart the DMA from the start of the buffer and the counter from INIT so the frame begins cleanly
  dmaReset(sm_index);
  flexpwm->SM[submodule].CTRL2 |= FLEXPWM_SMCTRL2_FRCEN;
  flexpwm->SM[submodule].CTRL2 |= FLEXPWM_SMCTRL2_FORCE;

  // TODO(LDOK): LDOK is only set once in flexpwmFrequency(). Verify on a scope that each DMA write to
  // VAL0..VAL5 is loaded every PWM period (FlexPWM VALDE DMA) without re-setting LDOK; not changed here.
  busy_submodules |= sm_bit;
  flexpwm->MCTRL |= FLEXPWM_MCTRL_RUN(1 << submodule);

  return DShotError_Success;
}

template<uint8_t submodule_number>
static void ISR() {
  constexpr unsigned int submodule = _SM(submodule_number);
  IMXRT_FLEXPWM_t *flexpwm = flexpwmModule(_FM(submodule_number));

  flexpwm->MCTRL &= ~FLEXPWM_MCTRL_RUN(1 << submodule);  // Clear only this submodule's RUN bit
  dma_channels[submodule_number]->clearInterrupt();
  busy_submodules &= ~(1 << submodule_number);
  asm("DSB");
}
