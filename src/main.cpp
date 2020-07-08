/**
 * Single Key Bluetooth Macro Keyboard
 * Author: Tim Jordan (mail@tim-jordan.de)
 */
#include <Arduino.h>
#include <BleKeyboard.h>

BleKeyboard bleKeyboard("Bluetooth Macro Pad", "derTim", 100);

#define BUTTON_COMMAND bleKeyboard.press(KEY_LEFT_GUI); bleKeyboard.press(KEY_F4);

#define BUTTON_PIN A10
#define BATTERY_PIN A13

unsigned long eventReadKeys;
#define READKEYS_WAITMS 50

unsigned long eventStatus;
#define STATUS_WAITMS 1000

unsigned long eventCheckBattery;
#define CHECKBATTERY_WAITMS 60000

unsigned long eventTimeout;
#define TIMEOUT_WAITMS 120000

int blink_interval;
bool last;
bool led_last;


int getBatteryPercentage(float fVoltage) {
  float fVoltageMatrix[22][2] = {
    {4.2,  100},
    {4.15, 95},
    {4.11, 90},
    {4.08, 85},
    {4.02, 80},
    {3.98, 75},
    {3.95, 70},
    {3.91, 65},
    {3.87, 60},
    {3.85, 55},
    {3.84, 50},
    {3.82, 45},
    {3.80, 40},
    {3.79, 35},
    {3.77, 30},
    {3.75, 25},
    {3.73, 20},
    {3.71, 15},
    {3.69, 10},
    {3.61, 5},
    {3.27, 0},
    {0, 0}
  };

  int i, perc;

  perc = 100;

  for(i=20; i>=0; i--) {
    if(fVoltageMatrix[i][0] >= fVoltage) {
      perc = fVoltageMatrix[i + 1][1];
      break;
    }
  }
  return perc;
}

/*
Method to print the reason by which ESP32
has been awaken from sleep
*/
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Bluetooth Macro Pad started!");
  bleKeyboard.begin();                                                                                                                                                                                                                                                                                                  

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  
  //Print the wakeup reason for ESP32
  print_wakeup_reason();

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);

  eventReadKeys = millis() + READKEYS_WAITMS;
  eventStatus = millis() + STATUS_WAITMS;
  eventCheckBattery = millis() + CHECKBATTERY_WAITMS / 10;
  eventTimeout = millis() + TIMEOUT_WAITMS;
  last = 1;
  blink_interval = 500;
}

void buttonPressed()
{
  Serial.println("press");
  digitalWrite(LED_BUILTIN, true);
  if(bleKeyboard.isConnected()) {
    BUTTON_COMMAND
    delay(25);
    bleKeyboard.releaseAll();
    eventTimeout = millis() + TIMEOUT_WAITMS * 10;
  }
  else {
    eventTimeout = millis() + TIMEOUT_WAITMS;
  }
  
  digitalWrite(LED_BUILTIN, false);
}

void loop() {
  if(eventReadKeys < millis())
  {
    bool val = digitalRead(BUTTON_PIN);
    if( val != last){
      //Serial.println("changed");
      last = val;
      if(val == 1){
        buttonPressed();
      }
    }
    eventReadKeys = millis() + READKEYS_WAITMS;
  }
  if(eventStatus < millis())
  {
    if (bleKeyboard.isConnected() && led_last == false){
      blink_interval = STATUS_WAITMS * 10;
    }
    else {
      blink_interval = STATUS_WAITMS;
    }
    digitalWrite(LED_BUILTIN, led_last);
    led_last = !led_last;
    eventStatus = millis() + blink_interval;
  }
  if(eventCheckBattery < millis())
  {
    int adcValue = analogRead(BATTERY_PIN);
    float batVoltage = ((float)adcValue/4095)*2*3.3*1.1;
    int batPercentage = getBatteryPercentage(batVoltage);
    bleKeyboard.setBatteryLevel(batPercentage);
    Serial.print("Battery: ");
    Serial.print(batVoltage);
    Serial.print("V ");
    Serial.print(batPercentage);
    Serial.println("%");
    eventCheckBattery = millis() + CHECKBATTERY_WAITMS;
  }
  if(eventTimeout < millis()){
    Serial.println("No BT Connection after 120s, or not used for 20min, going to sleep, wakeup by button press, or reset!");
    delay(1000);
    esp_deep_sleep_start();
  }
}