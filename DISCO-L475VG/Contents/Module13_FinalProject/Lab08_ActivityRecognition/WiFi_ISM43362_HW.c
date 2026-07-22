/* -----------------------------------------------------------------------------
 * Copyright (c) 2019-2020 Arm Limited (or its affiliates). All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 *
 * $Date:        13. February 2020
 * $Revision:    V1.1
 *
 * Project:      WiFi Driver Hardware specific implementation for
 *               Inventek ISM43362-M3G-L44 WiFi Module (SPI variant)
 * -------------------------------------------------------------------------- */

#include "stm32l4xx_hal.h"
#include <stdint.h>

// Add device specific include files here
// <code WiFi_ISM43362_include_files>

// </code>

/**
  \fn          void WiFi_ISM43362_Pin_Initialize (void)
  \brief       Initialize pin(s).
  \return      none
*/
void WiFi_ISM43362_Pin_Initialize(void) {
  // Add code for initializing Reset, Slave Select and Data Ready pins here
  // (for Data Ready pin external interrupt can also be setup here)
  // <code WiFi_ISM43362_Pin_Initialize>

  GPIO_InitTypeDef GPIO_Init;

  GPIO_Init.Pin = GPIO_PIN_1;
  GPIO_Init.Mode = GPIO_MODE_IT_RISING;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_Init);

  GPIO_Init.Pin = GPIO_PIN_8;
  GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_Init.Alternate = 0;
  HAL_GPIO_Init(GPIOE, &GPIO_Init);

  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, GPIO_PIN_SET);
  GPIO_Init.Pin = GPIO_PIN_0;
  GPIO_Init.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_Init.Pull = GPIO_NOPULL;
  GPIO_Init.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOE, &GPIO_Init);

  // </code>
}

/**
  \fn          void WiFi_ISM43362_Pin_Uninitialize (void)
  \brief       De-initialize pin(s).
  \return      none
*/
void WiFi_ISM43362_Pin_Uninitialize(void) {
  // Add code for deinitializing Reset, Slave Select and Data Ready pins here
  // <code WiFi_ISM43362_Pin_Uninitialize>

  // </code>
}

/**
  \fn          void WiFi_ISM43362_Pin_RSTN (uint8_t rstn)
  \brief       Drive Reset line.
  \param[in]   rstn
                 - value = 0: Drive Reset line not active state
                 - value = 1: Drive Reset line active state
  \return      none
*/
void WiFi_ISM43362_Pin_RSTN(uint8_t rstn) {
  // Add code for driving Reset pin here
  // <code WiFi_ISM43362_Pin_RSTN>
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, rstn ? GPIO_PIN_RESET : GPIO_PIN_SET);
  // </code>
}

/**
  \fn          void WiFi_ISM43362_Pin_SSN (uint8_t ssn)
  \brief       Drive Slave Select line.
  \param[in]   ssn
                 - value = 0: Drive Slave Select line not active state
                 - value = 1: Drive Slave Select line active state
  \return      none
*/
void WiFi_ISM43362_Pin_SSN(uint8_t ssn) {
  // Add code for driving Slave Select pin here
  // <code WiFi_ISM43362_Pin_SSN>
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_0, ssn ? GPIO_PIN_RESET : GPIO_PIN_SET);

  // </code>
}

/**
  \fn          uint8_t WiFi_ISM43362_Pin_DATARDY (void)
  \brief       Get Data Ready line state.
  \return      Data Ready line state
                 - 0: Data Ready line is not active state
                 - 1: Data Ready line is active state
*/
uint8_t WiFi_ISM43362_Pin_DATARDY(void) {
  // Add code for retrieving Data Ready pin state here
  // <code WiFi_ISM43362_Pin_DATARDY>
  return HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_1) == GPIO_PIN_SET;
  // </code>
}
