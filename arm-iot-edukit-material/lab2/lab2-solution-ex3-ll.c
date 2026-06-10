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
  printf("Low-level Blinky\n");

  volatile GPIO_PERIPHERAL *GPIO_B = (volatile GPIO_PERIPHERAL *)0x48000400;
  volatile GPIO_PERIPHERAL *GPIO_C = (volatile GPIO_PERIPHERAL *)0x48000800;

  GPIO_B->MODER &= ~(3 << 28);
  GPIO_B->MODER |= (1 << 28);

  GPIO_C->MODER &= ~(3 << 26);

  for (;;) {
    if (!!(GPIO_C->IDR & (1 << 13))) {
      GPIO_B->ODR &= ~(1 << 14);
    }
    else {
      GPIO_B->ODR |= 1 << 14;
    }
  }

  return 0;
}
