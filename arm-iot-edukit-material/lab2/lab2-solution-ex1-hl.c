#include "Driver_GPIO.h"
#include "GPIO_STM32.h"
#include <stdio.h>

typedef struct {
  unsigned int MODER;
  unsigned int OTYPER;
  unsigned int OSPEEDR;
  unsigned int PUPDR;
  unsigned int IDR;
  unsigned int ODR;
  unsigned int BSRRL;
  unsigned int BSRRH;
  unsigned int LCKR;
  unsigned int AFR[2];
} GPIO_PERIPHERAL;

int app_main(void) {
  printf("High-level Blinky\n");

  Driver_GPIO0.SetDirection(GPIO_PIN_ID_PORTB(14), ARM_GPIO_OUTPUT);

  for (;;) {
    Driver_GPIO0.SetOutput(GPIO_PIN_ID_PORTB(14), 1);

    for (unsigned int i = 0; i < 10000000; i++) {
      asm volatile("");
    }

    Driver_GPIO0.SetOutput(GPIO_PIN_ID_PORTB(14), 0);

    for (unsigned int i = 0; i < 10000000; i++) {
      asm volatile("");
    }
  }

  return 0;
}
