#include <Arduino.h>

// define output pins
#define LED2 2       // LED_BUILTIN
#define RED_LED 5    // gpio 5; pin 29
#define GREEN_LED 18 // gpio 18; pin 30
#define BLUE_LED 19  // gpio 19; pin 31

#define buttonInterruptPin 32 // function pushbutton; gpio 32; pin 7

#define functionBit0 12 // bit 0 (lsb) of function selected; gpio 12; pin 13
#define functionBit1 13 // bit 1 of function selected; gpio 13; pin 15
#define functionBit2 14 // bit 2 of function selected; gpio 14; pin 12
#define functionBit3 15 // bit 3 (msb) of function7 selected; gpio 15; pin 23

#define LED_ON LOW   // my LEDs are common collector (active LOW)
#define LED_OFF HIGH // my LEDs are common collector (active LOW)

int function = 0; // selected function (4 bits)
struct buttonInterruptLine
{
  const uint8_t PIN; // gpio pin number
  uint32_t numHits;  // number of times fired
  bool PRESSED;      // boolean logical; is triggered now
};
buttonInterruptLine buttonPress = {buttonInterruptPin, 0, false};

void IRAM_ATTR press()
{
  buttonPress.numHits++;
  buttonPress.PRESSED = true;
}
void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Starting setup()...");
  // config outputs
  gpio_config_t io_conf;
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pin_bit_mask = (1 << LED2) | (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
  gpio_config(&io_conf);
  // config function selector inputs (use unsigned long since 32 is long)
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pin_bit_mask = (1ULL << functionBit0) | (1ULL << functionBit1) | (1ULL << functionBit2) | (1ULL << functionBit3);
  gpio_config(&io_conf);

  // config function pushbutton interrupt
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_NEGEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  io_conf.pin_bit_mask = (1 << buttonPress.PIN);
  gpio_config(&io_conf);

  // pinMode(buttonPress.PIN, INPUT_PULLUP);
  attachInterrupt(buttonPress.PIN, press, FALLING);

  Serial.println("Done setup()");
}

void loop()
{
  if (buttonPress.PRESSED)
  {
    // read input pins, to set function
    function = digitalRead(functionBit0);
    function |= (digitalRead(functionBit1) << 1);
    function |= (digitalRead(functionBit2) << 2);
    function |= (digitalRead(functionBit3) << 3);
    function = (15 - function);
    Serial.printf("Selected: [%d]; fired %u times\n", function, buttonPress.numHits);

    buttonPress.PRESSED = false;
  }
  digitalWrite(LED2, LED_ON);
  GPIO.out_w1ts = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
  delay(500);
  digitalWrite(LED2, LED_OFF);
  GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
  delay(500);
}