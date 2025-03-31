#pragma once

#include <Arduino.h>
#include <SPI.h>

#define NUM_ROWS 16
#define NUM_COLS 64
#define NUM_LEDS ROWS *COLS
#define NUM_BLANK_CYCLES 0

#define OE_PIN 1    // !OE, can be tied to GND if need to save pin
#define LATCH_PIN 0 // RCLK

volatile uint64_t displayBuffer[NUM_ROWS] = {
    (uint64_t)0b0000000000000000000000000011111100000000000000111111110000000000,
    (uint64_t)0b0000000110000000000000001111110011000000000011111111111100000000,
    (uint64_t)0b0000001111000000000000010011110000100000000111100000011110000000,
    (uint64_t)0b0000001111000000000000100111111100010000001110001001000111000000,
    (uint64_t)0b0111111111111110000000101100001110010000001110001001000111000000,
    (uint64_t)0b0011110110111100000001111000000111111000001111000000001111000000,
    (uint64_t)0b0001110110111000000001111000000110011000000111111111111110000000,
    (uint64_t)0b0000111111110000000001011000000100001000000011111111111100000000,
    (uint64_t)0b0000111111110000000001001100001100001000000000111111110000000000,
    (uint64_t)0b0000111111110000000001001111111110011000000110001001000110000000,
    (uint64_t)0b0001111111111000000001011111111111111000001001101001011001000000,
    (uint64_t)0b0001111111111000000000111001001001110000001000111001100001000000,
    (uint64_t)0b0011111001111100000000010001001000100000001000001001000001000000,
    (uint64_t)0b0011100000011100000000010000000000100000000100000000000010000000,
    (uint64_t)0b0110000000000110000000001000000001000000000011000000001100000000,
    (uint64_t)0b0000000000000000000000000111111110000000000000111111110000000000,
};

// uint64_t fullRowData = ~displayBuffer[curLine];
// uint16_t rowData = (fullRowData << scrollIndex) | (fullRowData >> (NUM_COLS - scrollIndex));
// rowSelect = ~(0x01 << curLine);


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

  TCCR2A = 0;              // set entire TCCR2A register to 0
  TCCR2B = 0;              // same for TCCR2B
  TCNT2 = 0;               // initialize counter value to 0
  OCR2A = 249;             // = (16*10^6) / (8000*8) - 1 (must be <256)
  TCCR2A |= (1 << WGM21);  // turn on CTC mode
  TCCR2B |= (1 << CS21);   // Set CS21 bit for 8 prescaler
  TIMSK2 |= (1 << OCIE2A); // enable timer compare interrupt

  sei();
}

inline void setDisplayState(bool displayState)
{
  display = displayState; // update the display state
  digitalWrite(OE_PIN, !display);
}

inline void scroll()
{
  scrollIndex = (scrollIndex + 1) % NUM_COLS;
}

ISR(TIMER2_COMPA_vect)
{
  uint16_t rowData, rowSelect;

  if (blankCycles > 0 || !display)
  {
    rowData = 0xFFFF;
    rowSelect = 0xFFFF;
  }
  else
  {
    uint64_t fullRowData = ~displayBuffer[curLine];
    rowData = (fullRowData << scrollIndex) | (fullRowData >> (NUM_COLS - scrollIndex));
    rowSelect = ~(0x01 << curLine);
  }

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