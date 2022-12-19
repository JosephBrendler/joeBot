#include <Arduino.h>

// define gpios-- use consecutive GPIOs (consecutive io register bits)
//    so "new way" can write all of them with one instruction
#define RED 17
#define GRN 18
#define BLU 19

// config gpios
gpio_config_t io_conf;

void setup()
{

  // output mode - old way
  pinMode(RED, OUTPUT);
  pinMode(GRN, OUTPUT);
  pinMode(BLU, OUTPUT);

  // output mode - new way
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  // io_conf.pin_bit_mask = (1 << RED) | (1 << GRN) | (1 << BLU);   // or
  io_conf.pin_bit_mask = (0b111 << RED); // or
  // io_conf.pin_bit_mask = (0b11100000000000);
  gpio_config(&io_conf);
}

void old_way()
{
  digitalWrite(RED, HIGH);
  digitalWrite(GRN, HIGH);
  digitalWrite(BLU, HIGH);
  digitalWrite(RED, LOW);
  digitalWrite(GRN, LOW);
  digitalWrite(BLU, LOW);
}

void new_way()
{
  // GPIO.w1ts = (1 << RED) | (1 << GRN) | (1 << BLU); // or
  GPIO.out_w1ts = (0b111 << RED);
  // GPIO.out_w1tc = (1 << RED) | (1 << GRN) | (1 << BLU);  //or
  GPIO.out_w1tc = (0b111 << RED);
}

void loop() { new_way(); }
