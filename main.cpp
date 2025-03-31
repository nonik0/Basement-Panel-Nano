#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <SPI.h>

#define NUM_ROWS 16
#define NUM_COLS 16
#define NUM_LEDS ROWS *COLS
#define NUM_BLANK_CYCLES 0

#define EXT_DISPLAY_PIN 12 // external display pin
#define OE_PIN 1           // !OE, can be tied to GND if need to save pin
#define LATCH_PIN 0        // RCLK
// clock and data pins are defined in SPI library

#define RGB_LED_BRIGHTNESS 1
#define RGB_LED_PIN 2
#define RGB_LED_COUNT 3

bool statusLed = false;
uint32_t Yellow = Adafruit_NeoPixel::Color(6 * RGB_LED_BRIGHTNESS, 3 * RGB_LED_BRIGHTNESS, 0);
uint32_t Amber = Adafruit_NeoPixel::Color(6 * RGB_LED_BRIGHTNESS, 2 * RGB_LED_BRIGHTNESS, 0);
uint32_t Orange = Adafruit_NeoPixel::Color(6 * RGB_LED_BRIGHTNESS, 1 * RGB_LED_BRIGHTNESS, 0);
uint32_t Red = Adafruit_NeoPixel::Color(6 * RGB_LED_BRIGHTNESS, 0, 0);
uint32_t Colors[] = {Yellow, Amber, Orange, Red};
Adafruit_NeoPixel rgbLeds(RGB_LED_COUNT, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

volatile bool display = true;
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

uint16_t limiter = 0;
volatile int scrollIndex = 0;
volatile int curLine = 0;
volatile int blankCycles = 0; // between each line cycle

ISR(TIMER2_COMPA_vect)
{ // timer1 interrupt
  // if (limiter++ < 2)
  //    return;
  // limiter = 0;

  uint16_t rowData = (blankCycles > 0 || !display) ? 0xFFFF : ~displayBuffer[curLine];
  uint16_t rowSelect = (blankCycles > 0 || !display) ? 0xFFFF : ~(0x01 << curLine);

  // shift rowData by scrollIndex bits, wrapping bits around
  rowData = (rowData << scrollIndex) | (rowData >> (NUM_COLS - scrollIndex));

  SPI.transfer16(rowData);
  SPI.transfer16(rowSelect);
  digitalWrite(LATCH_PIN, LOW);
  digitalWrite(LATCH_PIN, HIGH);

  blankCycles--;

  if (blankCycles < 0)
  {
    curLine = (curLine + 1) % NUM_ROWS;
    // curLine--;
    // if (curLine < 0) {
    //   curLine = NUM_ROWS - 1;
    // }
    blankCycles = 2;
  }
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");

  pinMode(EXT_DISPLAY_PIN, INPUT_PULLUP);
  pinMode(OE_PIN, OUTPUT);
  pinMode(LATCH_PIN, OUTPUT);

  digitalWrite(OE_PIN, !display);
  digitalWrite(LATCH_PIN, LOW);

  SPI.begin();

  cli();

  // // set timer0 for interrupt at REFRESH_RATE_US us interval or REFRESH_RATE_US Hz
  // TCCR0A = TCCR0B = TCNT0 = 0;
  // OCR0A = 61; // 4kHz /w 64 prescaler
  // TCCR0A |= (1 << WGM01);
  // TCCR0B |= (1 << CS01) | (1 << CS00);
  // TIMSK0 |= (1 << OCIE0A);

  // set timer2 interrupt at 8kHz
  TCCR2A = 0; // set entire TCCR2A register to 0
  TCCR2B = 0; // same for TCCR2B
  TCNT2 = 0;  // initialize counter value to 0
  // set compare match register for 8khz increments
  OCR2A = 249; // = (16*10^6) / (8000*8) - 1 (must be <256)
  // turn on CTC mode
  TCCR2A |= (1 << WGM21);
  // Set CS21 bit for 8 prescaler
  TCCR2B |= (1 << CS21);
  // enable timer compare interrupt
  TIMSK2 |= (1 << OCIE2A);

  sei();

  rgbLeds.begin();

  Serial.println("Finished setup");
}

int colorIndex = 0;
int colorDelay = 2;
void loop()
{
  rgbLeds.clear();

  display = digitalRead(EXT_DISPLAY_PIN);
  digitalWrite(OE_PIN, !display);

  if (display)
  {
    for (int i = 0; i < RGB_LED_COUNT; i++)
    {
      rgbLeds.setPixelColor(i, Colors[(i + colorIndex) % 3]);
    }
  }
  rgbLeds.show();

  scrollIndex = (scrollIndex + 1) % NUM_COLS;
  if (--colorDelay <= 0)
  {
    colorDelay = 2;
    colorIndex = (colorIndex + 1) % 3;
  }
  delay(100);
  // scan animation test

  // // row scan
  // for (int row = 0; row < NUM_ROWS; row++) {
  //   for (int y = 0; y < NUM_ROWS; y++) {
  //     for (int x = 0; x < NUM_COLS; x++) {
  //       setPixel(x, y, y == row);
  //     }
  //   }
  //   delay(250);
  // }

  // // col scan
  // for (int col = 0; col < NUM_COLS; col++) {
  //   for (int x = 0; x < NUM_COLS; x++) {
  //     for (int y = 0; y < NUM_ROWS; y++) {
  //       setPixel(x, y, x == col);
  //     }
  //   }
  //   delay(250);
  // }

  // // diagonal scan
  // for (int diag = 0; diag < NUM_ROWS + NUM_COLS - 1; diag++) {
  //   for (int x = 0; x < NUM_COLS; x++) {
  //     for (int y = 0; y < NUM_ROWS; y++) {
  //       if (x + y == diag) {
  //         setPixel(x, y, 1);
  //       } else {
  //         setPixel(x, y, 0);
  //       }
  //     }
  //   }
  //   delay(250);
  // }
}