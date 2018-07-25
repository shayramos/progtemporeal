#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).

// Definições: 
//  1 - Tarefa que faz a leitura da luminosidade( Simula temperatura )
//  2 - Tarefa que faz a leitura da vazão 
//  3 - Calculo do fechamento da valvula ( relação temperatura x vazão = saida da valvula %)
//  4 - Monitor de segurança  - a

// Premissas
// Válvula resfria o trocador
// Saída do calculo é o percentual de abertura da válvula
// Para uma determinada temperatura o monitor de segurança toma controle da variável de saída  e manda um valor padrão
// Monitor de segurança atua quando for identificado um determinado valor de temperatura 
// toma o controle da saída  calculada e aciona led informando emergencia


// Temperatura ( 0 - 92)
// Vazão ( 0 - 99)
// Abertura da valvula (0 - 100)
SemaphoreHandle_t xSerialSemaphore;
SemaphoreHandle_t xSecuritySemaphore;

QueueHandle_t queueTemperatura;
QueueHandle_t queueVazao;
QueueHandle_t queueSecurity;
QueueHandle_t queueToOutput;
TaskHandle_t handleCalculadora;

void TaskLeituraVazao( void *pvParameters );
void TaskLeituraTemperatura( void *pvParameters );
void TaskMonitorSeguranca( void *pvParameters );
void TaskCalculador( void *pvParameters );
void TaskOutput (void * pvParameters );

int valorTemperatura = 0;
int valorVazao = 0;
int valorValvula = 100;
const int temperaturaLimite = 80;


 
// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  while (!Serial);

  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }
  if ( xSecuritySemaphore == NULL ) 
  {
    xSecuritySemaphore = xSemaphoreCreateMutex(); 
    if ( ( xSecuritySemaphore ) != NULL )
      xSemaphoreGive( ( xSecuritySemaphore ) ); 
  }

  if (queueTemperatura == NULL) {
    queueTemperatura = xQueueCreate(1, sizeof(int));
    if (!queueTemperatura) Serial.println("Não conseguiu criar a queue para temperatura!");
  }

  if (queueVazao == NULL) {
    queueVazao = xQueueCreate(1, sizeof(int));
    if (!queueVazao) Serial.println("Não conseguiu criar a queue da vazao!");
  }

  if (queueSecurity = NULL){
    queueSecurity = xQueueCreate(1,sizeof(int));
    if (!queueSecurity) Serial.println("Não conseguiu criar a queue para security");
  }
  
  xTaskCreate(
    TaskLeituraVazao
    ,  (const portCHAR *) "LeituraVazao"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );

  xTaskCreate(
    TaskLeituraTemperatura
    ,  (const portCHAR *) "LeituraTemperatura"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );

  xTaskCreate(
    TaskMonitorSeguranca
    ,  (const portCHAR *) "MonitorSeguranca"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  NULL );

  xTaskCreate(
    TaskCalculador
    ,  (const portCHAR *) "Calculadora"
    ,  128  // Stack size
    ,  NULL
    ,  1  // Priority
    ,  &handleCalculadora );

  // Now the Task scheduler, which takes over control of scheduling individual Tasks, is automatically started.
}

void loop()
{
  // Não será usado
}

/*--------------------------------------------------*/
/*---------------------- Tasks ---------------------*/
/*--------------------------------------------------*/


/*
Task que faz a leitura da temperatura
*/
void TaskLeituraTemperatura( void *pvParameters __attribute__((unused)) )  
{
  const TickType_t period = 25;
  TickType_t lastWakeTime;
  int sensorValue1;
  for (;;)
  {
    lastWakeTime = xTaskGetTickCount();
    sensorValue1 = analogRead(A1);
    sensorValue1 = (int)(((float)(sensorValue1)/1023)*100);
    xQueueSendToFront(queueTemperatura, &sensorValue1, 100);


    vTaskDelayUntil(&lastWakeTime, period); 
  }
}

/*
Task que faz a leitura da vazão
*/
void TaskLeituraVazao( void *pvParameters __attribute__((unused)) )  
{
  const TickType_t period = 25;
  TickType_t lastWakeTime;
  int sensorValue0;
  for (;;)
  {
    lastWakeTime = xTaskGetTickCount();
    sensorValue0 = analogRead(A0);
    sensorValue0 = (int)(((float)(sensorValue0)/1023)*100);
    xQueueSendToFront(queueVazao, &sensorValue0, 100);

    
    vTaskDelayUntil(&lastWakeTime, period);
  }
}

/*
Task que faz o calculo
*/
void TaskCalculador( void *pvParameters __attribute__((unused)) )  
{
  int currentVazao = 0;
  int currentTemp = 0;
  int saida;
  const TickType_t period = 25;
  TickType_t lastWakeTime;
  for (;;)
  {
    lastWakeTime = xTaskGetTickCount();
    if (xSemaphoreTake(xSecuritySemaphore, ( TickType_t ) 5) == pdTRUE) {
      int err;
      err = xQueueReceive(queueTemperatura, &currentTemp, 20);
      if (err == pdPASS) {
        err = xQueueReceive(queueVazao, &currentVazao, 20);
        if (err == pdPASS) {
          valorTemperatura = currentTemp;
          valorVazao = currentVazao;
          saida =  (valorTemperatura*valorTemperatura - valorVazao)/120 ; //sei la
          valorValvula = saida;
          
          
          if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
            Serial.println((const char*)"Temperatura:");
            Serial.println(valorTemperatura);
            Serial.println("Vazão");
            Serial.println(valorVazao);
            Serial.println("Valor valvula");
            Serial.println(valorValvula);
          
            
            xSemaphoreGive( xSerialSemaphore );
          }
          
          
        }
      }
      xSemaphoreGive(xSecuritySemaphore);
    }
   vTaskDelayUntil(&lastWakeTime, period);
  }
}

/*
Task que faz o calculo
*/
void TaskMonitorSeguranca( void *pvParameters __attribute__((unused)) )  
{
  const TickType_t period = 25;
  TickType_t lastWakeTime;
  int currentTemp, currentVazao;

  for (;;)
  {
    lastWakeTime = xTaskGetTickCount();
    if (valorTemperatura > temperaturaLimite) {
      if ( xSemaphoreTake( xSecuritySemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
        vTaskSuspend( handleCalculadora );
        valorValvula = 100;
        int err;
        err = xQueueReceive(queueTemperatura, &currentTemp, 20);
        if (err == pdPASS) {
          err = xQueueReceive(queueVazao, &currentVazao, 20);
          if (err == pdPASS) {
            valorTemperatura = currentTemp;
            valorVazao = currentVazao;
            if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ) {
              Serial.println((const char*)"Temperatura:");
              Serial.println(valorTemperatura);
              Serial.println("Vazão");
              Serial.println(valorVazao);
              Serial.println("Valor valvula");
              Serial.println(valorValvula);
          
            
            xSemaphoreGive( xSerialSemaphore );
          }
        }
        }
      }
      xSemaphoreGive(xSecuritySemaphore);
     
    }
    if (valorTemperatura <= temperaturaLimite) {
        vTaskResume(handleCalculadora);
    }
    
    vTaskDelayUntil(&lastWakeTime, period);
  }
}

void TaskOutput( void *pvParameters __attribute__((unused)) )  {
  const TickType_t period = 25;
  TickType_t lastWakeTime;
  for (;;) {
    lastWakeTime = xTaskGetTickCount();

    vTaskDelayUntil(&lastWakeTime, period);
  }
  
}

