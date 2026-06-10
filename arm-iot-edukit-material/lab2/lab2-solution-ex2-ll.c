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

  volatile GPIO_PERIPHERAL *GPIO_A = (volatile GPIO_PERIPHERAL *)0x48000000;
  volatile GPIO_PERIPHERAL *GPIO_B = (volatile GPIO_PERIPHERAL *)0x48000400;

  GPIO_A->MODER &= ~(3 << 10);
  GPIO_A->MODER |= (1 << 10);
  GPIO_B->MODER &= ~(3 << 28);
  GPIO_B->MODER |= (1 << 28);

  for (;;) {
    GPIO_B->ODR |= 1 << 14;
    GPIO_A->ODR &= ~(1 << 5);
    for (unsigned int i = 0; i < 10000000; i++) {
      asm volatile("");
    }

    GPIO_B->ODR &= ~(1 << 14);
    GPIO_A->ODR |= 1 << 5;
    for (unsigned int i = 0; i < 10000000; i++) {
      asm volatile("");
    }
  }

  return 0;
}
