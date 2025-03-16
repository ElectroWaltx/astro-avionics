#include <Arduino.h>
#include "E32.h"
#include "testEjectionCharges.h"
#include "bmp.h"

#define BMP_SDA A4
#define BMP_SCL A5
BMPSensor bmpSensor(BMP_SDA, BMP_SCL);

// define pins
#define BuzzerPin A1

// define Constants
#define BOOT_TIME 15000    // time to wait for both systems to boot up
#define CONNECT_TIME 15000 // time to wait for the connection to be established

// Define serial ports
HardwareSerial SerialE32(1);   // Use UART1 (A3 RX, A2 TX)
HardwareSerial SerialRaspi(2); // Use UART2 (A6 RX, A7 TX)
E32Module e32(SerialE32);

// define Gobal variables
unsigned long startTime = 0;
int connectionStatus = 0; // 0 if fail , 1 if success
float currentAltitude = 0;
float baseAltitude = 0;
float bmpVelocity = 0; // velocity of the rocket calculated by bmp altitude readings
int bmpWorking = 0;    // 0 if fail , 1 if success

// define task handle
TaskHandle_t buzzerTaskHandle;

// define buzzer task
void buzzerTask(void *pvParameters)
{
  // initialize the buzzer
  pinMode(BuzzerPin, OUTPUT);

  // we have to beep once every second till time is less than BOOT_TIME + CONNECT_TIME
  while (1)
  {
    unsigned long currentTime = millis();
    unsigned long elapsedTime = currentTime - startTime;

    if (connectionStatus == 1)
    {
      // beep for one second and then stop
      digitalWrite(BuzzerPin, HIGH);
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      digitalWrite(BuzzerPin, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    if (elapsedTime < BOOT_TIME)
    {
      digitalWrite(BuzzerPin, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(BuzzerPin, LOW);
      vTaskDelay(900 / portTICK_PERIOD_MS);
    }
    else if (elapsedTime < BOOT_TIME + CONNECT_TIME)
    {
      digitalWrite(BuzzerPin, HIGH);
      vTaskDelay(500 / portTICK_PERIOD_MS);
      digitalWrite(BuzzerPin, LOW);
      vTaskDelay(500 / portTICK_PERIOD_MS);
    }
    else if (connectionStatus == 0)
    {

      // beap fast if connection unsuccessful
      digitalWrite(BuzzerPin, HIGH);
      vTaskDelay(100 / portTICK_PERIOD_MS);
      digitalWrite(BuzzerPin, LOW);
      vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    else
    {
      vTaskDelete(NULL);
    }
  }
}

float getAltitude()
{
  // get the altitude from the bmp sensor
  // TODO: implement this function
  return 0;
}
void bmpTask(void *pvParameters)
{
  // initialize the sensor
  // TODO : set bmpworking 1 if intialized successfully , else 0

  // check if the sensor is working
  if (bmpWorking == 0)
  {
    // if the sensor is not working, set the base altitude to 0
    baseAltitude = 0;
    // delete task
    vTaskDelete(NULL);
  }
  else
  {
    // if the sensor is working, set the base altitude to the current altitude
    baseAltitude = getAltitude();
    // TODO : can take average of 10 readings every second to get the base altitude more accurately
  }
  while (millis() < BOOT_TIME)
  {
    // wait for the boot time to pass
    ;
  }

  unsigned long t1 = millis();
  unsigned long t2 = millis();
  // set the start time
  while (1)
  {
    // get the current time
    t2 = millis();
    // calculate the velocity of the rocket
    float reading = getAltitude();
    bmpVelocity = (reading - baseAltitude) / (t2 - t1);
    // get the current altitude every 10 ms
    currentAltitude = reading;

    // update the time
    t1 = t2;

    // wait for 10ms
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void rasPiTask(void *pvParameters)
{
  // wait for boot time
  while (millis() < BOOT_TIME)
  {
    ;
  }
  // establish connection and set connection status to 1 if successful
}

/**
 * @brief Arduino setup function.
 *
 * Initializes serial communication at 115200 baud rate, sets pin modes,
 * and creates tasks.
 *
 * @param BuzzerPin The pin number connected to the buzzer.
 * @param buzzerTask The task function to handle buzzer operations.
 * @param "buzzerTask" The name of the task.
 * @param 1024 The stack size allocated for the task.
 * @param NULL The parameter passed to the task (none in this case).
 * @param 1 The priority of the task.
 * @param NULL The task handle (not used in this case).
 */
void setup()
{
  Serial.begin(115200);
  SerialE32.begin(9600, SERIAL_8N1, A3, A2);     // E32 module
  SerialRaspi.begin(115200, SERIAL_8N1, A6, A7); // Raspberry Pi
  setupRelays();
  if (!bmpSensor.begin())
  {
    // TODO
    // switch to getting altitude data from imu readings
  }
  else
  {
    // if the sensor is working, set the base altitude to the current altitude
    baseAltitude = getAltitude();
    // TODO : can take average of 10 readings every second to get the base altitude more accurately
  }
 
  // set the start time

  // create tasks
  /*
   * @param "buzzerTask" The name of the task.
   * @param 1024 The stack size allocated for the task.
   * @param NULL The parameter passed to the task (none in this case).
   * @param 1 The priority of the task.
   * @param NULL The task handle (not used in this case).
   */
  xTaskCreate(buzzerTask, "buzzerTask", 1024, NULL, 1, &buzzerTaskHandle); // create buzzer task
  xTaskCreate(bmpTask, "bmpTask", 2048, NULL, 1, NULL);                    // create bmp task
  // NOTE: increase the stack size if required
  // NOTE: increase the priority if required
}

void loop()
{
  Serial.println("Sending message...");
  String message = "hello";
  e32.sendMessage(message);
  triggerRelay(relayMain);
  delay(1000); // Small delay before next relay signal

  triggerRelay(relayDrogue);
  delay(1000);

  triggerRelay(relayBackup);
  delay(1000);
  while (millis() < BOOT_TIME)
  {
    // wait for the boot time to pass
    ;
  }

  unsigned long t1 = millis();
  unsigned long t2 = millis();
  while (1)
  {
    // get the current time
    t2 = millis();
    // calculate the velocity of the rocket
    float reading = getAltitude();
    bmpVelocity = (reading - baseAltitude) / (t2 - t1);
    // get the current altitude every 10 ms
    currentAltitude = reading;

    // update the time
    t1 = t2;
    Serial.print("Altitude: ");
    Serial.print(currentAltitude - baseAltitude);
    Serial.println(" meters");
    delay(10);
  }
}