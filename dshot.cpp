#include "dshot.h"
#include "DMAChannel.h"

#ifndef __IMXRT1062__
#error THIS MODULE ONLY WORKS WITH TEENSY 4.0 AND 4.1
#endif

static void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, float frequency);

template<uint8_t pin> static void ISR();

// Direct from pwm.c

#include "imxrt.h"
#include "core_pins.h"

struct pwm_pin_info_struct {
  uint8_t type;     // 0=no pwm, 1=flexpwm, 2=quad
  uint8_t flex_module;   // 0-3, 0-3
  uint8_t channel;  // 0=X, 1=A, 2=B
  uint8_t muxval;   //
};

#define M(a, b) ((((a)-1) << 4) | (b))

#if defined(__IMXRT1062__)

static const struct pwm_pin_info_struct pwm_pin_info[] = {
  { 1, M(1, 1), 0, 4 },  // FlexPWM1_1_X   0  // AD_B0_03
  { 1, M(1, 0), 0, 4 },  // FlexPWM1_0_X   1  // AD_B0_02
  { 1, M(4, 2), 1, 1 },  // FlexPWM4_2_A   2  // EMC_04
  { 1, M(4, 2), 2, 1 },  // FlexPWM4_2_B   3  // EMC_05
  { 1, M(2, 0), 1, 1 },  // FlexPWM2_0_A   4  // EMC_06
  { 1, M(2, 1), 1, 1 },  // FlexPWM2_1_A   5  // EMC_08
  { 1, M(2, 2), 1, 2 },  // FlexPWM2_2_A   6  // B0_10
  { 1, M(1, 3), 2, 6 },  // FlexPWM1_3_B   7  // B1_01
  { 1, M(1, 3), 1, 6 },  // FlexPWM1_3_A   8  // B1_00
  { 1, M(2, 2), 2, 2 },  // FlexPWM2_2_B   9  // B0_11
  { 2, M(1, 0), 0, 1 },  // QuadTimer1_0  10  // B0_00
  { 2, M(1, 2), 0, 1 },  // QuadTimer1_2  11  // B0_02
  { 2, M(1, 1), 0, 1 },  // QuadTimer1_1  12  // B0_01
  { 2, M(2, 0), 0, 1 },  // QuadTimer2_0  13  // B0_03
  { 2, M(3, 2), 0, 1 },  // QuadTimer3_2  14  // AD_B1_02
  { 2, M(3, 3), 0, 1 },  // QuadTimer3_3  15  // AD_B1_03
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 2, M(3, 1), 0, 1 },  // QuadTimer3_1  18  // AD_B1_01
  { 2, M(3, 0), 0, 1 },  // QuadTimer3_0  19  // AD_B1_00
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 1, M(4, 0), 1, 1 },  // FlexPWM4_0_A  22  // AD_B1_08
  { 1, M(4, 1), 1, 1 },  // FlexPWM4_1_A  23  // AD_B1_09
  { 1, M(1, 2), 0, 4 },  // FlexPWM1_2_X  24  // AD_B0_12
  { 1, M(1, 3), 0, 4 },  // FlexPWM1_3_X  25  // AD_B0_13
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 1, M(3, 1), 2, 1 },  // FlexPWM3_1_B  28  // EMC_32
  { 1, M(3, 1), 1, 1 },  // FlexPWM3_1_A  29  // EMC_31
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 1, M(2, 0), 2, 1 },  // FlexPWM2_0_B  33  // EMC_07 //17
#ifdef ARDUINO_TEENSY40
  { 1, M(1, 1), 2, 1 },  // FlexPWM1_1_B  34  // SD_B0_03
  { 1, M(1, 1), 1, 1 },  // FlexPWM1_1_A  35  // SD_B0_02
  { 1, M(1, 0), 2, 1 },  // FlexPWM1_0_B  36  // SD_B0_01
  { 1, M(1, 0), 1, 1 },  // FlexPWM1_0_A  37  // SD_B0_00
  { 1, M(1, 2), 2, 1 },  // FlexPWM1_2_B  38  // SD_B0_05
  { 1, M(1, 2), 1, 1 },  // FlexPWM1_2_A  39  // SD_B0_04 //6
#endif
#ifdef ARDUINO_TEENSY41
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 1, M(2, 3), 1, 6 },  // FlexPWM2_3_A  36  // B1_00
  { 1, M(2, 3), 2, 6 },  // FlexPWM2_3_B  37  // B1_01
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 0, M(1, 0), 0, 0 },
  { 1, M(1, 1), 2, 1 },  // FlexPWM1_1_B  42  // SD_B0_03
  { 1, M(1, 1), 1, 1 },  // FlexPWM1_1_A  43  // SD_B0_02
  { 1, M(1, 0), 2, 1 },  // FlexPWM1_0_B  44  // SD_B0_01
  { 1, M(1, 0), 1, 1 },  // FlexPWM1_0_A  45  // SD_B0_00
  { 1, M(1, 2), 2, 1 },  // FlexPWM1_2_B  46  // SD_B0_05
  { 1, M(1, 2), 1, 1 },  // FlexPWM1_2_A  47  // SD_B0_04
  { 0, M(1, 0), 0, 0 },  // duplicate FlexPWM1_0_B
  { 0, M(1, 0), 0, 0 },  // duplicate FlexPWM1_2_A
  { 0, M(1, 0), 0, 0 },  // duplicate FlexPWM1_2_B
  { 1, M(3, 3), 2, 1 },  // FlexPWM3_3_B  51  // EMC_22
  { 0, M(1, 0), 0, 0 },  // duplicate FlexPWM1_1_B
  { 0, M(1, 0), 0, 0 },  // duplicate FlexPWM1_1_A
  { 1, M(3, 0), 1, 1 },  // FlexPWM3_0_A  54  // EMC_29 //10
#endif
#ifdef ARDUINO_TEENSY_MICROMOD
  { 1, M(1, 1), 2, 1 },  // FlexPWM1_1_B  34  // SD_B0_03
  { 1, M(1, 1), 1, 1 },  // FlexPWM1_1_A  35  // SD_B0_02
  { 1, M(1, 0), 2, 1 },  // FlexPWM1_0_B  36  // SD_B0_01
  { 1, M(1, 0), 1, 1 },  // FlexPWM1_0_A  37  // SD_B0_00
  { 1, M(1, 2), 1, 1 },  // FlexPWM1_2_A  38  // SD_B0_04
  { 1, M(1, 2), 2, 1 },  // FlexPWM1_2_B  39  // SD_B0_05
  { 2, M(2, 1), 0, 1 },  // QuadTimer2_1  40  // B0_04
  { 2, M(2, 2), 0, 1 },  // QuadTimer2_2  41  // B0_05
  { 0, M(1, 0), 0, 0 },  // duplicate QuadTimer3_0
  { 0, M(1, 0), 0, 0 },  // duplicate QuadTimer3_1
  { 0, M(1, 0), 0, 0 },  // duplicate QuadTimer3_2
  { 2, M(4, 0), 0, 1 },  // QuadTimer4_0  45  // B0_09
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

#if defined(ARDUINO_TEENSY41)
static void (*const dma_isr[CORE_NUM_DIGITAL])() = { ISR<0>, ISR<1>, ISR<2>, ISR<3>, ISR<4>, ISR<5>, ISR<6>, ISR<7>, ISR<8>, ISR<9>,
                                        ISR<10>, ISR<11>, ISR<12>, ISR<13>, ISR<14>, ISR<15>, ISR<16>, ISR<17>, ISR<18>, ISR<19>,
                                        ISR<20>, ISR<21>, ISR<22>, ISR<23>, ISR<24>, ISR<25>, ISR<26>, ISR<27>, ISR<28>, ISR<29>,
                                        ISR<30>, ISR<31>, ISR<32>, ISR<33>, ISR<34>, ISR<35>, ISR<36>, ISR<37>, ISR<38>, ISR<39>,
                                        ISR<40>, ISR<41>, ISR<42>, ISR<43>, ISR<44>, ISR<45>, ISR<46>, ISR<47>, ISR<48>, ISR<49>,
                                        ISR<50>, ISR<51>, ISR<52>, ISR<53>, ISR<54> };
#elif defined(ARDUINO_TEENSY_MICROMOD)
static void (*const dma_isr[CORE_NUM_DIGITAL])() = { ISR<0>, ISR<1>, ISR<2>, ISR<3>, ISR<4>, ISR<5>, ISR<6>, ISR<7>, ISR<8>, ISR<9>,
                                        ISR<10>, ISR<11>, ISR<12>, ISR<13>, ISR<14>, ISR<15>, ISR<16>, ISR<17>, ISR<18>, ISR<19>,
                                        ISR<20>, ISR<21>, ISR<22>, ISR<23>, ISR<24>, ISR<25>, ISR<26>, ISR<27>, ISR<28>, ISR<29>,
                                        ISR<30>, ISR<31>, ISR<32>, ISR<33>, ISR<34>, ISR<35>, ISR<36>, ISR<37>, ISR<38>, ISR<39>,
                                        ISR<40>, ISR<41>, ISR<42>, ISR<43>, ISR<44>, ISR<45> };
#elif defined(ARDUINO_TEENSY40)
static void (*const dma_isr[CORE_NUM_DIGITAL])() = { ISR<0>, ISR<1>, ISR<2>, ISR<3>, ISR<4>, ISR<5>, ISR<6>, ISR<7>, ISR<8>, ISR<9>,
                                        ISR<10>, ISR<11>, ISR<12>, ISR<13>, ISR<14>, ISR<15>, ISR<16>, ISR<17>, ISR<18>, ISR<19>,
                                        ISR<20>, ISR<21>, ISR<22>, ISR<23>, ISR<24>, ISR<25>, ISR<26>, ISR<27>, ISR<28>, ISR<29>,
                                        ISR<30>, ISR<31>, ISR<32>, ISR<33>, ISR<34>, ISR<35>, ISR<36>, ISR<37>, ISR<38>, ISR<39> };
#endif

#else
#error Unsupported IMXRT1062 board (expected Teensy 4.0, 4.1 or MicroMod)
#endif

static const float dshotFreq[DShotType_Count] = { 150'000, 300'000, 600'000, 1'200'000 };

static const uint16_t DATA_LENGTH = 20;
static const uint32_t BUSY_TIMEOUT_US = 1000;  // Longest frame (DShot150, 20 bits) is ~133us

static DMAChannel *dma_channels[CORE_NUM_DIGITAL] = { nullptr };
static uint16_t dma_data[CORE_NUM_DIGITAL][DATA_LENGTH] = {};
static float dshot_freq = dshotFreq[DShot150];
static uint64_t dma_pins = 0;                // "Bit array" for pin tracking
static volatile uint64_t busy_pins = 0;      // "Bit array" of pins with a frame in flight (cleared in ISR)
static uint16_t flexpwm_submodules = 0;  // "Bit array" for submodule tracking (1 << (submodule + 4*module))

float dshotFrequency() {
  //Serial.println("dshotFrequency()");
  return dshot_freq;
}

float dshotFrequency(uint8_t dshot_speed) {
  //Serial.println("dshotFrequency()");
  if (dshot_speed >= DShotType_Count) return -1.0f;  // Return an error if there is not compatable mode
  if (dma_pins > 0) return -2.0f;                    // Return and error if any pin is initialized TODO : uninitialization
  dshot_freq = dshotFreq[dshot_speed];
  return dshot_freq;
}

// Inspired by pwm.c : void analogWriteFrequency(uint8_t pin, float frequency)
static int dshotInit(uint8_t pin)  // Returns whether the pin is DShot capable (ie FlexPWM)
{
  //Serial.println("dshotInit()");
  const struct pwm_pin_info_struct *info;

  if (pin >= CORE_NUM_DIGITAL) return DShotError_NotAPin;  // Not even a pin!

  //Serial.printf("analogWriteFrequency, pin %d, freq %d\n", pin, (int)frequency);
  info = pwm_pin_info + pin;
  if (info->type == 1) {
    // FlexPWM pin
    pinMode(pin, OUTPUT);
    *(portConfigRegister(pin)) = info->muxval;
    unsigned int submodule = info->flex_module & 0x03;
    unsigned int flex_module = (info->flex_module >> 4) & 3;
    uint16_t mask = 1 << submodule;

    if (((flexpwm_submodules >> (submodule + 4 * flex_module)) & 1) != 0) {  // Can't have two pins on the same submodule
      return DShotError_SubmoduleUsed;
    }
    const uint16_t submodule_bit = 1 << (submodule + 4 * flex_module);

    IMXRT_FLEXPWM_t *flexpwm;
    switch (flex_module) {
      case 0: flexpwm = &IMXRT_FLEXPWM1; break;
      case 1: flexpwm = &IMXRT_FLEXPWM2; break;
      case 2: flexpwm = &IMXRT_FLEXPWM3; break;
      default: flexpwm = &IMXRT_FLEXPWM4;
    }


    flexpwmFrequency(flexpwm, submodule, info->channel, dshot_freq);

    switch (info->channel) {
      case 0:  // X
        flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMX_EN(mask);
        //Serial.println("X : Enable");
        break;
      case 1:  // A
        flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMA_EN(mask);
        //Serial.println("A : Enable");
        break;
      case 2:  // B
        flexpwm->OUTEN |= FLEXPWM_OUTEN_PWMB_EN(mask);
        //Serial.println("B : Enable");
    }


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

    DMAChannel *dma = new DMAChannel();
    if (dma->channel >= DMA_MAX_CHANNELS) {  // DMAChannel couldn't allocate a hardware channel
      delete dma;                            // Nothing has been claimed yet, so a later retry starts clean
      return DShotError_NoDMAAvailable;
    }
    dma_channels[pin] = dma;

    dma_channels[pin]->sourceBuffer(dma_data[pin], sizeof(dma_data[pin]));

    switch (info->channel) {
      case 0:  // X
        dma_channels[pin]->destination((uint16_t &)flexpwm->SM[submodule].VAL0);
        //Serial.println("X : dest()");
        break;
      case 1:  // A
        dma_channels[pin]->destination((uint16_t &)flexpwm->SM[submodule].VAL3);
        //Serial.println("A : dest()");
        break;
      case 2:  // B
        dma_channels[pin]->destination((uint16_t &)flexpwm->SM[submodule].VAL5);
        //Serial.println("B : dest()");
    }
    dma_channels[pin]->triggerAtHardwareEvent(dma_source);
    dma_channels[pin]->interruptAtCompletion();
    dma_channels[pin]->attachInterrupt(dma_isr[pin]);
    dma_channels[pin]->enable();

    flexpwm_submodules |= submodule_bit;  // Only claim the submodule once setup has fully succeeded
    return DShotError_Success;  // DShot capable
  }
  return DShotError_NotFlexPWM;  // Not DShot capable (QuadTimer)
}

// Inspired by pwm.c : flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel __attribute__((unused)), float frequency)
static void flexpwmFrequency(IMXRT_FLEXPWM_t *p, unsigned int submodule, uint8_t channel, float frequency) {
  //Serial.println("flexpwmFrequency()");
  uint16_t mask = 1 << submodule;
  uint32_t newdiv = (uint32_t)((float)F_BUS_ACTUAL / frequency + 0.5f);

  //Serial.printf(" div=%lu, scale=%lu\n", newdiv, 1);
  p->MCTRL |= FLEXPWM_MCTRL_CLDOK(mask);
  p->SM[submodule].CTRL = FLEXPWM_SMCTRL_FULL;  // | FLEXPWM_SMCTRL_PRSC(7);
  p->SM[submodule].VAL0 = 0;
  p->SM[submodule].VAL1 = newdiv - 1;
  p->SM[submodule].VAL2 = 0;  // For edge aligned
  p->SM[submodule].VAL3 = 0;
  p->SM[submodule].VAL4 = 0;  // For edge aligned
  p->SM[submodule].VAL5 = 0;
  p->MCTRL |= FLEXPWM_MCTRL_LDOK(mask);
  p->SM[submodule].DMAEN = FLEXPWM_SMDMAEN_VALDE;
}


// Inspired by pwm.c : void analogWrite(uint8_t pin, int val)
int sendDSHOT(uint8_t pin, uint16_t throttle, bool telemetry) {
  //Serial.println("sendDSHOT()");
  if (pin >= CORE_NUM_DIGITAL) return DShotError_NotAPin;

  if (((dma_pins >> pin) & 1) == 0) {
    int error = dshotInit(pin);
    if (error > 0) return error;
    dma_pins |= 1ULL << pin;  // 64-bit shift: pins >= 32 exist on Teensy 4.1 / MicroMod
  }

  //Serial.printf("sendDSHOT, pin %d, throttle %d, telemetry %c\n", pin, throttle, telemetry ? 'T' : 'F');

  const struct pwm_pin_info_struct *info;
  info = pwm_pin_info + pin;
  unsigned int submodule = info->flex_module & 0x03;

  if (info->type == 1) {
    // FlexPWM pin
    uint16_t data = (constrain(throttle, 0, DSHOT_THROTTLE_MAX) << 5) | (telemetry << 4);
    data |= ((data >> 4) ^ (data >> 8) ^ (data >> 12)) & 0x0f;

    IMXRT_FLEXPWM_t *flexpwm;
    switch ((info->flex_module >> 4) & 3) {
      case 0: flexpwm = &IMXRT_FLEXPWM1; break;
      case 1: flexpwm = &IMXRT_FLEXPWM2; break;
      case 2: flexpwm = &IMXRT_FLEXPWM3; break;
      default: flexpwm = &IMXRT_FLEXPWM4;
    }

    uint32_t bit_length = flexpwm->SM[submodule].VAL1;

    // Wait for any previous frame on this pin to finish before touching its buffer
    const uint64_t pin_bit = 1ULL << pin;
    elapsedMicros waited;
    while ((busy_pins & pin_bit) && waited < BUSY_TIMEOUT_US) {}
    if (busy_pins & pin_bit) {  // ISR never fired: stop the submodule and recover
      flexpwm->MCTRL &= ~FLEXPWM_MCTRL_RUN(1 << submodule);
      busy_pins &= ~pin_bit;
    }

    if (info->channel == 0) {  // X : inverted need to zero because we will flip later.
      for (int i = 0; i < DATA_LENGTH; i++) {
        dma_data[pin][i] = 0;
      }
    }

    for (int i = 0; i < 16; i++) {
      uint32_t offset = (data >> i) & 1;
      dma_data[pin][18 - i - 1] = (bit_length * 3) >> (3 - offset);  // High time: 3/8 (37.5%) for a 0 bit, 3/4 (75%) for a 1 bit
    }

    if (info->channel == 0) {  // X : inverted
      for (int i = 0; i < DATA_LENGTH; i++) {
        dma_data[pin][i] = bit_length - dma_data[pin][i];
      }
      //Serial.println("Inverted");
    }

    /*
    Serial.print("dma_data = [ ");
    for (int i = 0; i < DATA_LENGTH; i++) {
      Serial.print(dma_data[pin][i]);
      Serial.print(" ");
    }
    Serial.println("]");
    */

    // Restart the DMA from the start of the buffer and the counter from INIT so the frame begins cleanly
    dma_channels[pin]->sourceBuffer(dma_data[pin], sizeof(dma_data[pin]));
    flexpwm->SM[submodule].CTRL2 |= FLEXPWM_SMCTRL2_FRCEN;
    flexpwm->SM[submodule].CTRL2 |= FLEXPWM_SMCTRL2_FORCE;

    // TODO(LDOK): LDOK is only set once in flexpwmFrequency(). Verify on a scope that each DMA write to
    // VALx is loaded every PWM period (FlexPWM VALDE DMA) without re-setting LDOK; not changed here.
    busy_pins |= pin_bit;
    flexpwm->MCTRL |= FLEXPWM_MCTRL_RUN(1 << submodule);

    return DShotError_Success;
  }
  return DShotError_NotFlexPWM;
}

template<uint8_t pin>
static void ISR() {
  //Serial.print("ISR() : ");
  //Serial.println(pin);

  const struct pwm_pin_info_struct *info;
  info = pwm_pin_info + pin;

  unsigned int submodule = info->flex_module & 0x03;
  unsigned int flex_module = (info->flex_module >> 4) & 0x03;

  IMXRT_FLEXPWM_t *flexpwm;
  switch (flex_module) {
    case 0: flexpwm = &IMXRT_FLEXPWM1; break;
    case 1: flexpwm = &IMXRT_FLEXPWM2; break;
    case 2: flexpwm = &IMXRT_FLEXPWM3; break;
    default: flexpwm = &IMXRT_FLEXPWM4;
  }

  flexpwm->MCTRL &= ~FLEXPWM_MCTRL_RUN(1 << submodule);  // Clear only this submodule's RUN bit
  dma_channels[pin]->clearInterrupt();
  busy_pins &= ~(1ULL << pin);
  asm("DSB");
}