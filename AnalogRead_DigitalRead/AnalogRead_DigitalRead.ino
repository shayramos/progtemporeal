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

SemaphoreHandle_t xSerialSemaphore;

// define two Tasks for DigitalRead & AnalogRead
void TaskLeituraVazao( void *pvParameters );
void TaskLeituraTemperatura( void *pvParameters );
void TaskMonitorSeguranca( void *pvParameters );
void TaskCalculador( void *pvParameters );
int valorTemperatura = 0;
int valorVazao = 0;

 
// the setup function runs once when you press reset or power the board
void setup() {

  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }

  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
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
    ,  NULL );

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

  for (;;)
  {
    int sensorValue1 = analogRead(A1);

    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      Serial.println(sensorValue1);

      xSemaphoreGive( xSerialSemaphore );
    }

    vTaskDelay(10); 
  }
}

/*
Task que faz a leitura da vazão
*/
void TaskLeituraVazao( void *pvParameters __attribute__((unused)) )  
{

  for (;;)
  {
    int sensorValue0 = analogRead(A0);

    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ) 
    {
      Serial.println(sensorValue0);

      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }

    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}

/*
Task que faz o calculo
*/
void TaskCalculador( void *pvParameters __attribute__((unused)) )  
{

  for (;;)
  {
    //int sensorValue = analogRead(A1);

    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE ) 
    {
     // Serial.println(sensorValue);

      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }

    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}

/*
Task que faz o calculo
*/
void TaskMonitorSeguranca( void *pvParameters __attribute__((unused)) )  
{

  for (;;)
  {
    //int sensorValue = analogRead(A2);

    if ( xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 5 ) == pdTRUE )
    {
      //Serial.println(sensorValue);

      xSemaphoreGive( xSerialSemaphore ); // Now free or "Give" the Serial Port for others.
    }

    vTaskDelay(10);  // one tick delay (15ms) in between reads for stability
  }
}
