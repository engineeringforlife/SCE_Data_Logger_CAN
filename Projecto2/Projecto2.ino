#include "Arduino.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include <Wire.h>

//Broker MQTT Libraries
#include <WiFi.h>
#include <PubSubClient.h>
//Can Library
#include "esp32_can.h"

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
#define TFT_GREY 0x5AEB
//variável necessária paraos mostradores
int old_value[6] = { -1, -1, -1, -1, -1, -1};

#define mainDELAY_LOOP_COUNT    400000 //( 0xffffff )

//Prototipos das Tasks
static void vTFTPresentation( void *pvParameters );
static void vTFTDisplayValues( void *pvParameters );
static void CANRx( void *pvParameters );
static void CANTx( void *pvParameters );
static void vWifiKeepAlive( void *pvParameters );


extern QueueHandle_t callbackQueue;

QueueHandle_t xQueueBroker;
QueueHandle_t xDataQueue;
SemaphoreHandle_t xCountingSemaphore;
SemaphoreHandle_t xMutex;	//=0?
TaskHandle_t xTFTPresentationHandle;

typedef struct
{
  float ucValue1;
  float ucValue2;
  float ucValue3;
  int ucSource;
} xData;


/* Definições do LED */
#define PIN_LED     25
/* Defines do MQTT */
#define TOPIC_LED    "topic_led"
#define TOPIC_TEMP   "topic_temp"
#define TOPIC_LUM    "topic_lum"
#define TOPIC_ACCELL "topic_accell"
#define TOPIC_HUM    "topic_hum"
#define TOPIC_ALARM  "topic_alarm"


#define ID_MQTT  "43a21d06b4a0454c903ddeeb425e814f"
const char* SSID = "MEO-351390"; // SSID / nome da rede WI-FI que deseja se conectar
const char* PASSWORD = "df82e87bf3"; // Senha da rede WI-FI que deseja se conectar
const char* BROKER_MQTT = "broker.hivemq.com"; //URL do broker MQTT que se deseja utilizar
int BROKER_PORT = 1883; // Porta do Broker MQTT

//Variáveis e objetos globais
WiFiClient espClient; // Cria o objeto espClient
PubSubClient MQTT(espClient); // Instancia o Cliente MQTT passando o objeto espClient

//PROTOTIPES
void initWiFi(void);
void initMQTT(void);
void mqtt_callback(char* topic, byte* payload, unsigned int length);
void reconnectMQTT(void);
void reconnectWiFi(void);
void VerificaConexoesWiFIEMQTT(void);


void initWiFi(void)
{
    delay(10);
    Serial.println("------Conexao WI-FI------");
    Serial.print("Conectando-se na rede: ");
    Serial.println(SSID);
    Serial.println("Aguarde");
    reconnectWiFi();
}

/* Função: inicializa parâmetros de conexão MQTT(endereço do
 *         broker, porta e seta função de callback)
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void initMQTT(void)
{
    MQTT.setServer(BROKER_MQTT, BROKER_PORT);   //informa qual broker e porta deve ser conectado
    MQTT.setCallback(mqtt_callback);            //atribui função de callback (função chamada quando qualquer informação de um dos tópicos subescritos chega)
}

/* Função: função de callback
 *         esta função é chamada toda vez que uma informação de
 *         um dos tópicos subescritos chega)
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void mqtt_callback(char* topic, byte* payload, unsigned int length)
{

    xData xSenderStructure;
    String msg;
    portBASE_TYPE xStatus;

    /* obtem a string do payload recebido */
    for(int i = 0; i < length; i++)
    {
       char c = (char)payload[i];
       msg += c;
    }

    //a MESNSAGEM DEVE SER COLOCADA NUMA QUEUE, E A TAREFA CANTX DEVE SER DESBLOQUEADA
    Serial.print("Chegou a seguinte string via MQTT: ");
    Serial.println(msg);

    /* toma ação dependendo da string recebida */
    if (msg.equals("1"))
    {
    	xSenderStructure.ucSource= 6;
    	xSenderStructure.ucValue1=1;
        Serial.print("LED aceso mediante comando MQTT");
    }else if(msg.equals("0"))
    {
    	xSenderStructure.ucSource= 6;
		xSenderStructure.ucValue1=0;
        Serial.print("LED apagado mediante comando MQTT");
    }
    xStatus = xQueueSendToBack( xQueueBroker, &xSenderStructure, 0 );
    if ( xStatus != pdTRUE )
    {
      Serial.print( "mqtt_callback could not send to the queue.\r\n" );
    }else{
      Serial.print( "mqtt_callback send to the queue------------------------------.\r\n" );
    }

}

/* Função: reconecta-se ao broker MQTT (caso ainda não esteja conectado ou em caso de a conexão cair)
 *         em caso de sucesso na conexão ou reconexão, o subscribe dos tópicos é refeito.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void reconnectMQTT(void)
{
    while (!MQTT.connected())
    {
        Serial.print("* Tentando se conectar ao Broker MQTT: ");
        Serial.println(BROKER_MQTT);
        if (MQTT.connect(ID_MQTT))
        {
            Serial.println("Conectado com sucesso ao broker MQTT!");
            MQTT.subscribe(TOPIC_LED);
        }
        else
        {
            Serial.println("Falha ao reconectar no broker.");
            Serial.println("Havera nova tentatica de conexao em 2s");
            delay(2000);
        }
    }
}

/* Função: verifica o estado das conexões WiFI e ao broker MQTT.
 *         Em caso de desconexão (qualquer uma das duas), a conexão
 *         é refeita.
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void VerificaConexoesWiFIEMQTT(void)
{
    if (!MQTT.connected())
        reconnectMQTT(); //se não há conexão com o Broker, a conexão é refeita

     reconnectWiFi(); //se não há conexão com o WiFI, a conexão é refeita
}

/* Função: reconecta-se ao WiFi
 * Parâmetros: nenhum
 * Retorno: nenhum
 */
void reconnectWiFi(void)
{
    //se já está conectado a rede WI-FI, nada é feito.
    //Caso contrário, são efetuadas tentativas de conexão
    if (WiFi.status() == WL_CONNECTED)
        return;
    WiFi.begin(SSID, PASSWORD); // Conecta na rede WI-FI
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(100);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("Conectado com sucesso na rede ");
    Serial.print(SSID);
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
}

/*Função que desenha os mostradores
 * Fornecida nos exemplos do TFT
 */
void plotLinear(char *label, int x, int y)
{
  int w = 36;
  tft.drawRect(x, y, w, 155, TFT_GREY);
  tft.fillRect(x + 2, y + 19, w - 3, 155 - 38, TFT_WHITE);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(label, x + w / 2, y + 2, 2);
  for (int i = 0; i < 110; i += 10)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 6, TFT_BLACK);
  }
  for (int i = 0; i < 110; i += 50)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 9, TFT_BLACK);
  }
  tft.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 - 5, TFT_RED);
  tft.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 + 5, TFT_RED);
  tft.drawCentreString("---", x + w / 2, y + 155 - 18, 2);
}

/*Função que ajusta os ponteiros nos mostradores
 * A função foi alterada face à versão exemplo do arduino
 *
 * */
void plotPointer(int i, int sValue)
{
  int dy = 187;
  byte pw = 16;
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // Move the 6 pointers one pixel towards new value
  //for (int i = 0; i < 6; i++){
    char buf[8]; dtostrf(sValue, 4, 0, buf);
    tft.drawRightString(buf, i * 40 + 36 - 5, 187 - 27 + 155 - 18, 2);
    int dx = 3 + 40 * i;
    if (sValue < 0)   sValue = 0; // Limit value to emulate needle end stops
    if (sValue > 100) sValue = 100;

    while (!(sValue == old_value[i])) {
      dy = 187 + 100 - old_value[i];
      if (old_value[i] > sValue)
      {
        tft.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_value[i]--;
        tft.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        tft.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_value[i]++;
        tft.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
    }
 // }
 // }
}

/*Função que imprime a mensagem CAN
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

//	static portBASE_TYPE xHigherPriorityTaskWoken;
//	  xHigherPriorityTaskWoken = pdFALSE;
  Serial.print("gotHundred ");
 // xSemaphoreGiveFromISR( xCountingSemaphore, (BaseType_t*)&xHigherPriorityTaskWoken );
 // printFrame(frame);
}




/*Contém todas as configurações necessárias
 */
void setup( void )
{

  //Inicialização da porta série(debug
  Serial.begin(115200);
  while(!Serial);

  xDataQueue = xQueueCreate( 10, sizeof( xData ) );

  xQueueBroker = xQueueCreate( 5, sizeof( xData ) );
  xMutex = xSemaphoreCreateMutex();

  xCountingSemaphore = xSemaphoreCreateCounting( 10, 0 );

  //Inicialização do barramento CAN
  Serial.println("Initializing CAN");
  pinMode(GPIO_NUM_16, OUTPUT);
  digitalWrite(GPIO_NUM_16, LOW); //enable CAN transceiver
  // void ESP32CAN::setCANPins(gpio_num_t rxPin, gpio_num_t txPin)
  CAN0.setCANPins(GPIO_NUM_4, GPIO_NUM_5);
  //CAN0.begin(500000);
  CAN0.init(500000);
  Serial.println("RECEIVER Ready ...!");

  //CAN0.watchFor(0x00, 0x0A); //setup a special filter
  CAN0.watchFor(); //then let everything else through anyway
  CAN0.setCallback(0, gotHundred); //callback on that first special filter

  Serial.print( "Vamos ver se começa aqui\n" );
  //Inicia o display TFT
  tft.init();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  /* Inicializa a conexao wi-fi */
  initWiFi();
  /* Inicializa a conexao ao broker MQTT */
  initMQTT();
  VerificaConexoesWiFIEMQTT();
  MQTT.subscribe(TOPIC_LED);
  //MQTT.loop();

  if ( xDataQueue != NULL ){
	  if( xMutex != NULL ){
		  if(xCountingSemaphore != NULL){
			  if(xQueueBroker != NULL){
				  Serial.print( "Success\n" );
				  xTaskCreatePinnedToCore( vTFTPresentation, "vTFTPresentation", 1024, NULL, 2, &xTFTPresentationHandle, 1);
			  }else{
				  Serial.print( "The queueBroker not be created.\r\n" );
			  }
		  }else{
			  Serial.print( "The counting semaphore could not be created\r\n" );
		  }
	  }else{
		  Serial.print( "The mutex not be created.\r\n" );
	  }
  }else
  {
	  Serial.print( "The queue not be created.\r\n" );
  }
}

void loop()
{
  vTaskDelete( NULL );
}

/*
 * Tarefa responsavel pelo envio da mendsagem CAN
 * Desbloqueia assim que existam dados na Queue xQueueBroker
 */
static void CANTx( void *pvParameters )
{
  xData xReceivedStructure;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();

  for ( ;; )
  {
	    /* Wait for a message to arrive. */
	    xQueueReceive( xQueueBroker, &xReceivedStructure, portMAX_DELAY );
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



/*Esta tarefa é a primeira a correr neste nó e faz a apresentação
 * do grupo, espera 10 segundos, de seguida cria as restantes tarefas
 * e apaga-se a si mesma.
 */
static void vTFTPresentation( void *pvParameters )
{

  	  //Strings de apresentação do projecto
	  tft.drawString("Este projecto tem o ", 0, 0, 4);
	  tft.drawString("nome de \"data- ", 0, 25, 4);
	  tft.drawString("-logguer CAN\" e foi", 0, 50, 4);
	  tft.drawString("desenvolvido para a", 0, 75, 4);
	  tft.drawString("cadeira de SCE por:", 0, 100, 4);
	  tft.drawString("David Drumond", 15, 175, 4);
	  tft.drawString("Edgar Paulo", 15, 200, 4);

	  delay(10000);
	  //Limpa o ecrã
	  tft.fillScreen(TFT_BLACK);

	  Serial.print("\n********************************************************\n" );
	  Serial.print("\nvTFTPresentation is running and about to delete itself\r\n" );
	  //Create other tasks here
      xTaskCreatePinnedToCore( vTFTDisplayValues, "vTFTDisplayValues", 8196, NULL, 1, NULL, 0);
      xTaskCreatePinnedToCore( CANRx, "CANRx", 4098, NULL, 1, NULL, 1);
      xTaskCreatePinnedToCore( CANTx, "CANTx", 4098, NULL, 1, NULL, 1);
      xTaskCreatePinnedToCore( vWifiKeepAlive, "vWifiKeepAlive", 4098, NULL, 1, NULL, 1);
	  vTaskDelete(xTFTPresentationHandle);
}

/*
 * Esta tarefa corre a cada 2 segundos, apra manter a ligação ao broke estável
 */
static void vWifiKeepAlive( void *pvParameters )
{
  const TickType_t xTicksToWait = 2000 / portTICK_PERIOD_MS;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount();


  for ( ;; )
  {
	  Serial.print("\nMQTT RECONECT***************************************\r\n" );
	  VerificaConexoesWiFIEMQTT();
	  MQTT.loop();

	  vTaskDelayUntil( &xLastWakeTime, xTicksToWait);
  }
}

/*
 * Tarefa que recebe a mesnsagem CAN
 * Só corre Quando existem dados na Queue callbackQueue
 * Reencaminha os dados para a Queue xDataQueue
 */
static void CANRx( void *pvParameters )
{
  xData xSenderStructure;
  CAN_FRAME msg;

  for ( ;; )
  {

	  //Nesta fase já recebe os dados filtrados
      if(xQueueReceive(callbackQueue, &msg, portMAX_DELAY)==pdTRUE)
      {
      	Serial.println("task_LowLevelRX Drumond");
      	printFrame(&msg);

      	xSenderStructure.ucSource=msg.id;
      	xSenderStructure.ucValue1 = msg.data.uint8[0];
      	xSenderStructure.ucValue2 = msg.data.uint8[1];
      	xSenderStructure.ucValue3 = msg.data.uint8[2];

      	xQueueSend(xDataQueue, &xSenderStructure, 0 );

      }
  }
}


/*
 * Ajusta os ponteiros do display TFT e publica os valores
 * dos sensores nos tópicos correspondentes do MQTT
 */
static void vTFTDisplayValues( void *pvParameters )
{
  xData xSenderStructure;
  int i=1;
  char accell[10];
  char lum[5];
  char hum[5];
  char temp[5];

  byte d = 40;
  plotLinear("TEMP", 0, 160);
  plotLinear("HUM", 1 * d, 160);
  plotLinear("LUM", 2 * d, 160);
  plotLinear("X", 3 * d, 160);
  plotLinear("Y", 4 * d, 160);
  plotLinear("Z", 5 * d, 160);
  Serial.print("\nvTFTDisplayValues\n" );

  for ( ;; )
  {


      if(xQueueReceive(xDataQueue, &xSenderStructure, portMAX_DELAY)==pdTRUE)
      {
    	  VerificaConexoesWiFIEMQTT();
    	  MQTT.loop();
    	  Serial.print("QueueReceive(xQueuexData, &xSenderStructure, portMAX_DELAY)==pdTRUE\n" );
    	  switch(xSenderStructure.ucSource) {
    	    case 1:

    	    	Serial.print("\nValor 1 publicado no topico\n" );
    	    	plotPointer(3, int(xSenderStructure.ucValue1));
    	    	plotPointer(4, int(xSenderStructure.ucValue2));
    	    	plotPointer(5, int(xSenderStructure.ucValue3));
    	    	sprintf(accell, "(%d, %d, %d)", int(xSenderStructure.ucValue1), int(xSenderStructure.ucValue2), int(xSenderStructure.ucValue3) );
    	    	MQTT.publish(TOPIC_ACCELL, accell );
    	      break;
    	    case 2:
    	    	Serial.print("\nValor 2 publicado no topico\n" );
    	    	plotPointer(0, int(xSenderStructure.ucValue1));
    	    	sprintf(temp, "%d", int(xSenderStructure.ucValue1));
    	    	MQTT.publish(TOPIC_TEMP, temp );

    	      break;
    	    case 3:
    	    	Serial.print("\nValor 3 publicado no topico\n" );
    	    	plotPointer(1, int(xSenderStructure.ucValue1));
    	    	sprintf(hum, "%d", int(xSenderStructure.ucValue1));
				MQTT.publish(TOPIC_HUM, hum );
    	      break;
    	    case 4:
    	    	Serial.print("\nValor 4 publicado no topico\n" );
    	    	plotPointer(2, int(xSenderStructure.ucValue1));
    	    	sprintf(lum, "%d", int(xSenderStructure.ucValue1));
    	    	MQTT.publish(TOPIC_LUM, lum );
    	      break;
    	    case 5:
    	    	i++;
    	    	Serial.print("\nFoi gerada uma interrupção externa");
    	    	if(i>1){
    	    		i=0;
    	    		tft.drawString("Alarme ON! ", 0, 0, 4);
    	    		MQTT.publish(TOPIC_ALARM, "Alarme ON!" );

    	    	}else{
    	    		MQTT.publish(TOPIC_ALARM, "Alarme OFF!" );
    	    		tft.fillRect(0, 0, 240, 25,TFT_BLACK );
    	    		Serial.print("\n**********************************************************************\n****************************************************\n");
    	    	}

    	      break;
    	    default:
    	      // code block
    	    	Serial.print("\nDefault\n" );
    	  }
      }



  }
}




