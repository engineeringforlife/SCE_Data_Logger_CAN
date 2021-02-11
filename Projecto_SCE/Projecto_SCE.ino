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

#define mainDELAY_LOOP_COUNT    400000 //( 0xffffff )

// SI7006-A20 I2C address is 0x40(64)
#define AddrSI7006 0x40
#define AddrLTR329AL 0x29

#define mainSENDER_1    1
#define mainSENDER_2    2
#define mainSENDER_3    3

char tempComand = 0xF3;
char humComand = 0xF5;


static void vGetHum( void *pvParameters );
static void vGetTemp( void *pvParameters );
static void vGetLum( void *pvParameters );
static void vGetAccell( void *pvParameters );
static void vCanTx( void *pvParameters );
static void vMyCanRx( void *pvParameters );
static void vManageValues( void *pvParameters );


/* The service routine for the interrupt.  This is the interrupt that the task
will be synchronized with. */
static void IRAM_ATTR vExternalInterruptHandler( void );

// pin to generate interrupts
const uint8_t interruptPin = 12;
const uint8_t LED_BUILTIN = 2;
/*-----------------------------------------------------------*/

QueueHandle_t xI2CQueue;
extern QueueHandle_t callbackQueue;
QueueHandle_t xDataQueue;
SemaphoreHandle_t xMutex;

typedef struct
{
  float ucValue1;
  float ucValue2;
  float ucValue3;
  int ucSource;
} xData;

LIS2HH12 lis = LIS2HH12();


/*Imprime a mensagem CAN
 *
 */
void printFrame(CAN_FRAME *message)
{
  Serial.print(message->id, HEX);
  if (message->extended) Serial.print(" X ");
  else Serial.print(" S ");
  Serial.print(message->length, DEC);
  Serial.print(" ");
  for (int i = 0; i < message->length; i++) {
    Serial.print(message->data.byte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

void gotHundred(CAN_FRAME *frame)
{

  Serial.print("gotHundred ");
}

/*
 * Configurações iniciais
 */
void setup( void )
{
  Serial.begin(115200);
  while(!Serial);

  pinMode(interruptPin, INPUT_PULLUP);
  pinMode(15, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(interruptPin), &vExternalInterruptHandler, CHANGE);

  Serial.println("Initializing CAN");
  pinMode(GPIO_NUM_16, OUTPUT);
  digitalWrite(GPIO_NUM_16, LOW); //enable CAN transceiver
  CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5);
  CAN0.init(500000);

  Serial.println("SENDER Ready ...!");

  CAN0.watchFor(); //then let everything else through anyway
  CAN0.setCallback(0, gotHundred); //callback on that first special filter

  Wire.begin();
  Wire.beginTransmission(AddrSI7006);
  Wire.endTransmission();
  delay(300);

  //Wire.begin();
  Wire.beginTransmission(AddrLTR329AL);
  Wire.write(0x80); //control register
  Wire.write(0x01); //gain=1, active mode
  Wire.endTransmission();
  delay(500);

  lis.begin();
  lis.setBasicConfig();
  xI2CQueue = xQueueCreate( 5, sizeof( xData ) );
  xMutex = xSemaphoreCreateMutex();
  xDataQueue = xQueueCreate( 10, sizeof( xData ) );
  if ( xI2CQueue != NULL )
  {
	  if( xMutex != NULL ){
	      xTaskCreatePinnedToCore( vGetHum, "vGetHum", 2048, NULL, 2, NULL, 1);
	      xTaskCreatePinnedToCore( vGetTemp, "vGetTemp", 2048, NULL, 2, NULL, 1);
	      xTaskCreatePinnedToCore( vGetLum, "vGetLum", 2048, NULL, 2, NULL, 1);
	      xTaskCreatePinnedToCore( vGetAccell, "vGetAccell", 2048, NULL, 2, NULL, 1);
	      xTaskCreatePinnedToCore( vCanTx, "vCanTx", 1024, NULL, 1, NULL, 0);	//core 0 e 1???
	      xTaskCreatePinnedToCore( vMyCanRx, "vMyCanRx", 4096, NULL, 1, NULL, 0);
	      xTaskCreatePinnedToCore( vManageValues, "vManageValues", 2048, NULL, 1, NULL, 0);

	  }else{
		  Serial.print( "The mutex not be created.\r\n" );
	  }
  }
  else
  {
	  Serial.print( "The queue not be created.\r\n" );
  }
}

void loop()
{
  vTaskDelete( NULL );
}
/*
 * Handler da Interrupção externa
 */
static void  IRAM_ATTR  vExternalInterruptHandler( void )
{
static signed portBASE_TYPE xHigherPriorityTaskWoken;
xData xSenderStructure;

  xHigherPriorityTaskWoken = pdFALSE;

  xSenderStructure.ucSource = 0x05;
  xSenderStructure.ucValue1 =0x01;

  xQueueSendToFrontFromISR( xI2CQueue, &xSenderStructure, 0);

  Serial.print( "An Interrupt was generated!!!!!!!!11.\r\n" );
  if( xHigherPriorityTaskWoken == pdTRUE )
  {
	  Serial.print( "Context Switch!!!!!!!11.\r\n" );
	  portYIELD_FROM_ISR();
  }
}

/*
 * Lé o valor do sensor de humidade
 * Periodo de 1s
 */
static void vGetHum( void *pvParameters )
{
  xData xSenderStructure;
  portBASE_TYPE xStatus;
  const TickType_t xTicksToWait = 50 / portTICK_PERIOD_MS;
  float humidity =0.0;
  uint8_t dataT[2] = {0};
  uint8_t dataH[2] = {0};
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for ( ;; )
  {

	  if( xSemaphoreTake( xMutex, portMAX_DELAY)){
		  Serial.print( "vSenderSI7006_HUM got access.\r\n" );
		    // Start I2C transmission
		    Wire.beginTransmission(AddrSI7006);
		    // Send HUMIDITY measurement command, NO HOLD MASTER
		    Wire.write(0xF5);
		    // Stop I2C transmission
		    Wire.endTransmission();
		    delay(50);
		    // Request 2 bytes of data
		    Wire.requestFrom(AddrSI7006, 2);
		    // Read 2 bytes of data
		    // humidity msb, humidity lsb
		    if(Wire.available() == 2)
		    {
		      dataH[0] = Wire.read();
		      dataH[1] = Wire.read();
		    }

		    xSemaphoreGive(xMutex);
		    // Convert the data
		    humidity  = ((dataH[0] * 256.0) + dataH[1]);
		    humidity = ((125 * humidity) / 65536.0) - 6;

		    xSenderStructure.ucSource = 0x03;
		    //xSenderStructure.ucValue1 = (float)millis();
		    xSenderStructure.ucValue1 =int(humidity);
	    xStatus = xQueueSendToBack( xI2CQueue, &xSenderStructure, xTicksToWait );

	    if ( xStatus != pdPASS )
	    {
	      Serial.print( "SI7006_HUM Could not send to the queue.\r\n" );
	    }
	  }else{
		  Serial.print( "vSenderSI7006_HUM couldn't access.\r\n" );
		  }

    vTaskDelayUntil( &xLastWakeTime, ( 1000/portTICK_PERIOD_MS ) );
  }
}


/*
 * Lê o sensor de temperatura
 * Periodo de 1s
 */
static void vGetTemp( void *pvParameters )
{
  xData xSenderStructure;
  portBASE_TYPE xStatus;
  const TickType_t xTicksToWait = 500 / portTICK_PERIOD_MS;
  float humidity =0.0;
  float temp= 0.0;
  float ctemp = 0.0;
  uint8_t dataT[2] = {0};
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();


  for ( ;; )
  {

	  if( xSemaphoreTake( xMutex, portMAX_DELAY)){
		  Serial.print( "vGetTemp got access.\r\n" );
		    // Start I2C transmission
			Wire.beginTransmission(AddrSI7006);
			// Send temperature measurement command, NO HOLD MASTER
			Wire.write(0xF3);
			// Stop I2C transmission
			Wire.endTransmission();
			delay(10);
			// Request 2 bytes of data
			Wire.requestFrom(AddrSI7006, 2);
			// Read 2 bytes of data
			// temp msb, temp lsb
			if(Wire.available() == 2)
			{
			  dataT[0] = Wire.read();
			  dataT[1] = Wire.read();
			}
			xSemaphoreGive(xMutex);

			// Convert the data
			temp  = ((dataT[0] * 256.0) + dataT[1]);
			ctemp = ((175.72 * temp) / 65536.0) - 46.85;
		    xSenderStructure.ucSource = 0x02;
		    //xSenderStructure.ucValue1 = (float)millis();;
		    xSenderStructure.ucValue1 = int(ctemp);
	    xStatus = xQueueSendToBack( xI2CQueue, &xSenderStructure, xTicksToWait );

	    if ( xStatus != pdPASS )
	    {
	      Serial.print( "vGetTemp could not send to the queue.\r\n" );
	    }
	  }else{
		  Serial.print( "vGetTemp failed to got access.\r\n" );
	  }
	  vTaskDelayUntil( &xLastWakeTime, ( 1000/portTICK_PERIOD_MS ) );

  }
}

/*
 * Lê o valor da luminusidade
 * Periodo de 1s
 */
static void vGetLum( void *pvParameters )
{
  xData xSenderStructure;
  portBASE_TYPE xStatus;
  const TickType_t xTicksToWait = 500 / portTICK_PERIOD_MS;

  byte msb=0, lsb=0;
  uint16_t l;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

		  for ( ;; )
		  {
			  if( xSemaphoreTake( xMutex, portMAX_DELAY)){
					  Serial.print( "vGetLum got access.\r\n" );

					  //channel 0
					  Wire.beginTransmission(AddrLTR329AL);
					  Wire.write(0x8A); //low
					  Wire.endTransmission();
					  Wire.requestFrom((uint8_t)AddrLTR329AL, (uint8_t)1);
					  delay(10);
					  if(Wire.available())
						lsb = Wire.read();

					  Wire.beginTransmission(AddrLTR329AL);
					  Wire.write(0x8B); //high
					  Wire.endTransmission();
					  Wire.requestFrom((uint8_t)AddrLTR329AL, (uint8_t)1);
					  delay(10);
					  if(Wire.available())
						msb = Wire.read();
						xSemaphoreGive(xMutex);

					  l = (msb<<8) | lsb;
					  Serial.print( "ABOUT TO PINT CHANNEL OF LTR329ALS01\r\n" );
					  Serial.println(l, DEC); //output in steps (16bit)
					xSenderStructure.ucSource = 0x04;
					//xSenderStructure.ucValue1 = (float)millis();
					xSenderStructure.ucValue1 = l;
					xStatus = xQueueSendToBack( xI2CQueue, &xSenderStructure, xTicksToWait );
					if ( xStatus != pdPASS )
					{
					  Serial.print( "LTR329ALS01 Could not send to the queue.\r\n" );
					}
				}else{
				  Serial.print( "vGetLum failed to got access.\r\n" );
				}
			  vTaskDelayUntil( &xLastWakeTime, ( 1000/portTICK_PERIOD_MS ) );
		  }
}

/*
 * lê o valor da aceleração
 * Periodo de 0.5s
 */

static void vGetAccell( void *pvParameters )
{
  xData xSenderStructure;
  portBASE_TYPE xStatus;
  const TickType_t xTicksToWait = 500 / portTICK_PERIOD_MS;
  float x=1, y=2, z=3;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for ( ;; )
  {
	  if( xSemaphoreTake( xMutex, xTicksToWait)){
		  Serial.print( "vGetAccell got access.\r\n" );
		    lis.getAccel(&x, &y, &z);
		    xSemaphoreGive(xMutex);
		    xSenderStructure.ucSource = 0x01;
		   // xSenderStructure.ucValue1 = (float)millis();
		    xSenderStructure.ucValue1 = x;
		    xSenderStructure.ucValue2 = y;
		    xSenderStructure.ucValue3 = z;
	    xStatus = xQueueSendToBack( xI2CQueue, &xSenderStructure, xTicksToWait );
	    if ( xStatus != pdPASS )
	    {
	    	Serial.print( "LIS2HH12 Could not send to the queue.\r\n" );
	    }else{
	    	Serial.print( "LIS2HH12 has sent to the queue.\r\n" );
	    }
	  }else{
		  Serial.print( "vGetAccell failed to got access.\r\n" );
	  }
    vTaskDelayUntil( &xLastWakeTime, ( 1000/portTICK_PERIOD_MS ) );
  }
}


/*-----------------------------------------------------------*/


/*
 * Envia a mensagem CAN
 * Desbloqueia sempre que tem dados na QUEUE xI2CQueue
 */
static void vCanTx( void *pvParameters )
{
  xData xReceivedStructure;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for ( ;; )
  {
	  //exemplo do exercício 16
	    /* Wait for a message to arrive. */
	    xQueueReceive( xI2CQueue, &xReceivedStructure, 0 );
	    /* There is no need to check the return value as the task will block
	    indefinitely and only run again when a message has arrived.  When the
	    next line is executed there will be a message to be output. */

	  CAN_FRAME txFrame;
	  txFrame.rtr = 0;
	  txFrame.id = int(xReceivedStructure.ucSource);
	  txFrame.extended = false;
	  txFrame.length = 3;
	  txFrame.data.uint8[0] = int(xReceivedStructure.ucValue1);
	  txFrame.data.uint8[1] = int(xReceivedStructure.ucValue2);
	  txFrame.data.uint8[2] = int(xReceivedStructure.ucValue3);


	    Serial.println("SENT ...!");

	  Serial.println(xReceivedStructure.ucSource);
	  Serial.println(xReceivedStructure.ucValue1);
	  CAN0.sendFrame(txFrame);
	  vTaskDelayUntil( &xLastWakeTime, ( 100/portTICK_PERIOD_MS ) );

  }
}

/*
 * Lé a mesnsagem CAN e reencaminha para a Queue xDataQueue.
 * Desbloqueia sempre que tiver dados na Queue callbackQueue
 */
static void vMyCanRx( void *pvParameters )
{
  xData xDataStruct;
  CAN_FRAME msg;
  //CAN_frame_t rxFrame;
  portBASE_TYPE xStatus;
  const TickType_t xTicksToWait = 50 / portTICK_PERIOD_MS;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();


  for ( ;; )
  {

	  //Nesta fase já recebe os dados filtrados
      if(xQueueReceive(callbackQueue, &msg, portMAX_DELAY)==pdTRUE)
      {
      	Serial.println("task_LowLevelRX Drumond***************************************");
      	Serial.println("task_LowLevelRX Drumond******************************************************************");
      	printFrame(&msg);
      	xDataStruct.ucSource=msg.id;
      	xDataStruct.ucValue1 = msg.data.uint8[0];
      	xDataStruct.ucValue2 = msg.data.uint8[1];
      	xDataStruct.ucValue3 = msg.data.uint8[2];

      	xQueueSend(xDataQueue, &xDataStruct, 0 );

      }
  }
}
/*
 * Desbloqueia sempre que tiver dados na Queue xDataQueue
 * Permite Ligar ou desligar o LED quando é recebida uma mensagem.
 */
static void vManageValues( void *pvParameters )
{
  xData xSenderStructure;
  portBASE_TYPE xStatus;
  int i=1;
  const TickType_t xTicksToWait = 50 / portTICK_PERIOD_MS;
  char accell[10];
  char lum[5];
  char hum[5];
  char temp[5];
  uint8_t dataT[2] = {0};
  uint8_t dataH[2] = {0};
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();


  for ( ;; )
  {
      if(xQueueReceive(xDataQueue, &xSenderStructure, portMAX_DELAY)==pdTRUE){
    	  if(xSenderStructure.ucSource == 6){
    		  Serial.print("\n*****************************************\n" );
    		  Serial.print("\nLigar LED\n" );
    		  Serial.print("\n*****************************************\n" );

    		  digitalWrite(15, uint8_t(xSenderStructure.ucValue1));
    		  Serial.print("\n*****************************************\n" );
      }

  }
}
}



//------------------------------------------------------------------------------

