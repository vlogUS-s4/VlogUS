#include <delta.h>
#include <Arduino.h>

Delta deltabot;

void TC3_Handler()
{
  // Clear the interrupt flag
  TC3->COUNT16.INTFLAG.bit.MC0 = 1;
  // Call updateStepper directly in ISR
  deltabot.updateStepper();
}

void setupTimer()
{
  // Enable TC3 clock
  GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(TC3_GCLK_ID) | GCLK_CLKCTRL_GEN_GCLK0 | GCLK_CLKCTRL_CLKEN;
  while (GCLK->STATUS.bit.SYNCBUSY)
    ;

  // Disable TC3 before configuration
  TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY)
    ;

  // Configure TC3 for 50 kHz (20 µs period)
  // SAMD21 at 48 MHz, prescaler 1, count to 960 = 48 MHz / 960 = 50 kHz
  TC3->COUNT16.CTRLA.reg = TC_CTRLA_MODE_COUNT16 | TC_CTRLA_PRESCALER_DIV1;
  TC3->COUNT16.CC[0].reg = 959; // 48 MHz / 50 kHz - 1
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY)
    ;

  // Enable interrupt on match
  TC3->COUNT16.INTENSET.bit.MC0 = 1;

  // Enable TC3
  TC3->COUNT16.CTRLA.reg |= TC_CTRLA_ENABLE;
  while (TC3->COUNT16.STATUS.bit.SYNCBUSY)
    ;

  // Enable interrupt in NVIC
  NVIC_EnableIRQ(TC3_IRQn);
}

void setup()
{
  delay(2000); // Delay to allow reading messages on the console
  // DEBUG_SERIAL.begin(115200); // Debug serial at 115200 baud
  while (!DEBUG_SERIAL)
    ; // Wait for serial to be ready
  // DEBUG_SERIAL.println("Starting position control with 50 kHz timer...");

  Serial.begin(9600); // USB port for debugging

  deltabot.setup();
  setupTimer(); // Initialize the 50 kHz timer
}

void loop()
{
  deltabot.readAngleCommand();
}