/*---------------------------------------------------------------------------
 * Copyright (c) 2024 Arm Limited (or its affiliates).
 * All rights reserved.
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
 *---------------------------------------------------------------------------*/

#include <stdio.h>

#include "b_l475e_iot01a1_bus.h"
#include "lsm6dsl.h"
#include "lsm6dsl_reg.h"
#include "main.h"

#include "Driver_Common.h"
#include "Driver_WiFi.h"
#include "cmsis_os2.h" // ::CMSIS:RTOS2
#include "stm32l4xx_hal.h"

#include "core_mqtt.h"
#include "core_mqtt_serializer.h"

static LSM6DSL_Object_t lsm6dsl_obj;
extern ARM_DRIVER_WIFI Driver_WiFi0;

static MQTTContext_t mqttContext;

static void Init_Sensors() {
  LSM6DSL_IO_t lsm6dsl_io;

  lsm6dsl_io.BusType = LSM6DSL_I2C_BUS;
  lsm6dsl_io.Init = BSP_I2C2_Init;
  lsm6dsl_io.DeInit = BSP_I2C2_DeInit;
  lsm6dsl_io.WriteReg = BSP_I2C2_WriteReg;
  lsm6dsl_io.ReadReg = BSP_I2C2_ReadReg;
  lsm6dsl_io.GetTick = BSP_GetTick;
  lsm6dsl_io.Address = LSM6DSL_I2C_ADD_L;

  LSM6DSL_RegisterBusIO(&lsm6dsl_obj, &lsm6dsl_io);
  LSM6DSL_Init(&lsm6dsl_obj);
  LSM6DSL_ACC_Enable(&lsm6dsl_obj);
  LSM6DSL_ACC_SetOutputDataRate(&lsm6dsl_obj, 1);
  LSM6DSL_GYRO_Enable(&lsm6dsl_obj);
  LSM6DSL_GYRO_SetOutputDataRate(&lsm6dsl_obj, 1);
}

static void Init_WiFi() {
  // Initialise the Wi-Fi device.
  printf("wi-fi: initialising...\n");
  int32_t rc = Driver_WiFi0.Initialize(NULL);
  if (rc) {
    printf("Wi-Fi initialization failed: %d\n", rc);
    for (;;) {
    }
  }

  Driver_WiFi0.PowerControl(ARM_POWER_FULL);

  // Connect to a wireless network

  // ** IMPORTANT! ** //
  // You must change these settings to your own Wi-Fi network configuration
  ARM_WIFI_CONFIG_t wifiConfig = {0};
  wifiConfig.ssid = "spn-ud_2G";                // Your network SSID.
  wifiConfig.pass = "meeW3iequooghain";         // Your network password.
  wifiConfig.security = ARM_WIFI_SECURITY_WPA2; // Your network security mode.
  wifiConfig.ch = 0;
  wifiConfig.wps_method = ARM_WIFI_WPS_METHOD_NONE;

  // Activate the Wi-Fi connection.
  printf("wi-fi: connecting to ssid '%s'...\n", wifiConfig.ssid);
  rc = Driver_WiFi0.Activate(0, &wifiConfig);
  if (rc) {
    printf("Wi-Fi activation failed: %d\n", rc);
    for (;;) {
    }
  }
}

// Static buffer for MQTT data storage.
static uint8_t mqttBufferStorage[2048];
static MQTTFixedBuffer_t mqttBuffer = {mqttBufferStorage,
                                       sizeof(mqttBufferStorage)};

// Represents a network connection.
struct NetworkContext {
  ARM_DRIVER_WIFI *iface;
  int32_t socket;
};

/*
 * Receives data from the WiFi connection.
 */
static int32_t transportRecv(NetworkContext_t *pNetworkContext, void *pBuffer,
                             size_t bytesToRecv) {
  return pNetworkContext->iface->SocketRecv(pNetworkContext->socket, pBuffer,
                                            bytesToRecv);
}

/*
 * Sends data over the WiFi connection.
 */
static int32_t transportSend(NetworkContext_t *pNetworkContext,
                             const void *pBuffer, size_t bytesToSend) {
  return pNetworkContext->iface->SocketSend(pNetworkContext->socket, pBuffer,
                                            bytesToSend);
}

/*
 * Called for incoming MQTT messages.
 */

static _Bool mqttUserCallback(struct MQTTContext *pContext,
                              struct MQTTPacketInfo *pPacketInfo,
                              struct MQTTDeserializedInfo *pDeserializedInfo,
                              enum MQTTSuccessFailReasonCode *reason,
                              struct MQTTPropBuilder *propBuilder1,
                              struct MQTTPropBuilder *propBuilder2) {
  // Not used.

  return 0;
}

/*
 * Returns the current system time in milliseconds.
 */
static uint32_t mqttGetTime() {

  return (1000 * osKernelGetTickCount()) / osKernelGetTickFreq();
}

// A unique MQTT client identifier
#define MQTT_CLIENT_IDENTIFIER "lab4-iot-client"

static void Init_MQTT() {
  // Create a socket to connect to the server
  printf("wi-fi: creating comms socket...\n");
  int32_t brokerConnectionSocket = Driver_WiFi0.SocketCreate(
      ARM_SOCKET_AF_INET, ARM_SOCKET_SOCK_STREAM, ARM_SOCKET_IPPROTO_TCP);
  if (brokerConnectionSocket < 0) {
    printf("Socket creation failed: %d\n", brokerConnectionSocket);
    for (;;) {
    }
  }

  // ** IMPORTANT! ** //
  // You must change this IP address to the IP address of your virtual machine
  // which you can get from the AWS (or otherwise) console.
  uint8_t ip[4] = {1, 2, 3, 4};

  printf("wi-fi: establishing tcp connection to broker...\n");
  int32_t src =
      Driver_WiFi0.SocketConnect(brokerConnectionSocket, ip, sizeof(ip), 1883);

  if (src) {
    printf("Socket connection failed: %d\n", src);
    for (;;) {
    }
  }

  // Network connection details.
  NetworkContext_t mqttNetwork = {&Driver_WiFi0, brokerConnectionSocket};

  // Routines for communicating over the Wi-Fi connection.
  TransportInterface_t mqttTransport = {0};
  mqttTransport.pNetworkContext = &mqttNetwork;
  mqttTransport.recv = transportRecv;
  mqttTransport.send = transportSend;

  // Initialise the MQTT subsystem.
  printf("mqtt: initialising...\n");
  MQTTStatus_t mrc = MQTT_Init(&mqttContext, &mqttTransport, mqttGetTime,
                               mqttUserCallback, &mqttBuffer);
  if (mrc) {
    printf("MQTT initialisation failed: %s\n", MQTT_Status_strerror(mrc));
    for (;;) {
    }
  }

  // Establish a connection to the broker.
  MQTTConnectInfo_t connectInfo = {0};
  connectInfo.cleanSession = true;
  connectInfo.pClientIdentifier = MQTT_CLIENT_IDENTIFIER;
  connectInfo.clientIdentifierLength = strlen(connectInfo.pClientIdentifier);
  connectInfo.keepAliveSeconds = 60;

  bool sessionPresent = false;
  printf("mqtt: connecting...\n");
  mrc = MQTT_Connect(&mqttContext, &connectInfo, NULL, 1000, &sessionPresent,
                     NULL, NULL);
  if (mrc) {
    printf("MQTT connection failed: %s\n", MQTT_Status_strerror(mrc));
    for (;;) {
    }
  }
}

__NO_RETURN void app_main_thread(void *argument) {

  Init_Sensors();
  Init_WiFi();
  Init_MQTT();

  MQTTStatus_t mrc;

  for (;;) {
    LSM6DSL_Axes_t accelerometer, gyro;

    LSM6DSL_ACC_GetAxes(&lsm6dsl_obj, &accelerometer);
    LSM6DSL_GYRO_GetAxes(&lsm6dsl_obj, &gyro);

    printf("acc:  x=%d y=%d z=%d\n", accelerometer.x, accelerometer.y,
           accelerometer.z);
    printf("gyro: x=%d y=%d z=%d\n", gyro.x, gyro.y, gyro.z);

    char payload[256];
    snprintf(payload, sizeof(payload),
             "{ \"acc\": { \"x\": %d, \"y\": %d, \"z\": %d }, \"gyro\": { "
             "\"x\": %d, \"y\": %d, \"z\": %d } }",
             accelerometer.x, accelerometer.y, accelerometer.z, gyro.x, gyro.y,
             gyro.z);

    // Send a message to the broker.
    uint16_t pid = MQTT_GetPacketId(&mqttContext);
    MQTTPublishInfo_t publishInfo = {0};

    publishInfo.pTopicName = "arm/iot/lab8";
    publishInfo.topicNameLength = strlen(publishInfo.pTopicName);
    publishInfo.pPayload = payload;
    publishInfo.payloadLength = strlen(payload);

    printf("mqtt: publishing...\n");
    mrc = MQTT_Publish(&mqttContext, &publishInfo, pid, NULL);
    if (mrc) {
      printf("MQTT publish failed: %s\n", MQTT_Status_strerror(mrc));
      for (;;) {
      }
    }

    HAL_Delay(1000);
  }
}

/*-----------------------------------------------------------------------------
 * Application initialization
 *----------------------------------------------------------------------------*/
int app_main(void) {
  osKernelInitialize(); /* Initialize CMSIS-RTOS2 */
  osThreadNew(app_main_thread, NULL, NULL);
  osKernelStart(); /* Start thread execution */
  return 0;
}
