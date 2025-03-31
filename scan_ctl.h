#pragma once

#include <Arduino.h>
#include <SPI.h>

#define NUM_ROWS 16
#define NUM_COLS 16
#define NUM_LEDS ROWS *COLS
#define NUM_BLANK_CYCLES 0

#define OE_PIN 1           // !OE, can be tied to GND if need to save pin
#define LATCH_PIN 0        // RCLK

volatile uint16_t displayBuffer[NUM_ROWS] = {
    (uint16_t)0b0000000000000000,
    (uint16_t)0b0000000110000000,
    (uint16_t)0b0000001111000000,
    (uint16_t)0b0000001111000000,
    (uint16_t)0b0111111111111110,
    (uint16_t)0b0011110110111100,
    (uint16_t)0b0001110110111000,
    (uint16_t)0b0000111111110000,
    (uint16_t)0b0000111111110000,
    (uint16_t)0b0000111111110000,
    (uint16_t)0b0001111111111000,
    (uint16_t)0b0001111111111000,
    (uint16_t)0b0011111001111100,
    (uint16_t)0b0011100000011100,
    (uint16_t)0b0110000000000110,
    (uint16_t)0b0000000000000000,
};

volatile int curLine = 0;
volatile int blankCycles = 0; // between each line cycle
bool display;
volatile int scrollIndex;

void initScanCtl()
{
  pinMode(OE_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  digitalWrite(OE_PIN, LOW);
  digitalWrite(LATCH_PIN, LOW);

  SPI.begin();

  cli();

  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2 = 0;  // initialize counter value to 0
  OCR2A = 249; // = (16*10^6) / (8000*8) - 1 (must be <256)
  TCCR2A |= (1 << WGM21); // turn on CTC mode
  TCCR2B |= (1 << CS21);  // Set CS21 bit for 8 prescaler
  TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt

  sei();
}

inline void setDisplayState(bool displayState) {
  display = displayState; // update the display state
  digitalWrite(OE_PIN, !display);
}

inline void scroll() {
  scrollIndex = (scrollIndex + 1) % NUM_COLS;
}

ISR(TIMER2_COMPA_vect)
{
  uint16_t rowData = (blankCycles > 0 || !display) ? 0xFFFF : ~displayBuffer[curLine];
  uint16_t rowSelect = (blankCycles > 0 || !display) ? 0xFFFF : ~(0x01 << curLine);

  rowData = (rowData << scrollIndex) | (rowData >> (NUM_COLS - scrollIndex));

  SPI.transfer16(rowData);
  SPI.transfer16(rowSelect);
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(LATCH_PIN, HIGH);

  blankCycles--;

  if (blankCycles < 0)
  {
    curLine = (curLine + 1) % NUM_ROWS;
    blankCycles = 2;
  }
}