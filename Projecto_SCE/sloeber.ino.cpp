#ifdef __IN_ECLIPSE__
//This is a automatic generated file
//Please do not modify this file
//If you touch this file your change will be overwritten during the next build
//This file has been generated on 2021-02-11 05:53:49

#include "Arduino.h"
#include "Arduino.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <Wire.h>
#include <rhio-LIS2HH12.h>
#include <esp32_can.h>
#include <SPI.h>

void printFrame(CAN_FRAME *message) ;
void gotHundred(CAN_FRAME *frame) ;
void setup( void ) ;
void loop() ;
static void  IRAM_ATTR  vExternalInterruptHandler( void ) ;
static void vGetHum( void *pvParameters ) ;
static void vGetTemp( void *pvParameters ) ;
static void vGetLum( void *pvParameters ) ;
static void vGetAccell( void *pvParameters ) ;
static void vCanTx( void *pvParameters ) ;
static void vMyCanRx( void *pvParameters ) ;
static void vManageValues( void *pvParameters ) ;

#include "Projecto_SCE.ino"


#endif
