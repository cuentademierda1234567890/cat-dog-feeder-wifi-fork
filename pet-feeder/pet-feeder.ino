// Wifi and Blynk

#define BLYNK_TEMPLATE_ID "TMPL29s6kB3zH"
#define BLYNK_TEMPLATE_NAME "Pet Feeder"
#define BLYNK_AUTH_TOKEN "D0vcl-H8mgNZZHs_anNblgVs6wXCIYgv"
#define BLYNK_FIRMWARE_VERSION "0.1.0"

#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
char auth[] = BLYNK_AUTH_TOKEN;
char ssid[] = "TP-Link_0B8A";
char pass[] = "94095364";

WiFiEventHandler gotIpEventHandler, disconnectedEventHandler;
int checkPeriod = 1800000; // Check Blynk connection every 30 minutes (1800000 millisecond)
unsigned long currTime = 0;
bool BlynkStatus = false;
int blynktimevalue;

// Blynk connection check
void CheckBlynk(){
  if (WiFi.status() == WL_CONNECTED) {
    BlynkStatus = Blynk.connected();
    if(!BlynkStatus == 1){
      Blynk.config(auth, BLYNK_DEFAULT_DOMAIN, BLYNK_DEFAULT_PORT);
      Blynk.connect(5000);
    }
  } else {
    Blynk.disconnect();
  }
}

// EEPROM
#include <EEPROM.h>
#define EEPROM_SIZE 512

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo motor;

long defaultTime = 21600;
int multiplier = 3600;
bool status = 1;
bool settingsactive = 0;
long feedtime = defaultTime;
long selectedTime = 0;
long second, minute, hour, eeprom_second, eeprom_minute, eeprom_hour;
bool bylnkbtn = 0;
bool restartbtn = 0;

#define pot A0
#define reset D7

long eeprom_val;
long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);

  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

void eeprom() {
  EEPROMWritelong(0, feedtime);
  EEPROM.commit();
}



// Punto de entrada del programa
void setup()
{
  Serial.begin(115200);

  gotIpEventHandler = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP& event){});
  disconnectedEventHandler = WiFi.onStationModeDisconnected([](const WiFiEventStationModeDisconnected& event){});
  WiFi.begin(ssid, pass);
  delay(5000);

  EEPROM.begin(512);

  lcd.init();
  lcd.backlight();
  motor.write(0);
  lcd.clear();
  pinMode(reset, INPUT_PULLUP);
  lcd.begin(16, 2);
  motor.attach(D6, 500, 2400);
  lcd.setCursor(5, 0);
  lcd.print("PET");
  lcd.setCursor(4, 1);
  lcd.print("FEEDER");
  delay(2000);
  CheckBlynk();
}

void loop()
{
  if(millis() >= currTime + checkPeriod){
      currTime += checkPeriod;
      CheckBlynk();
  }

  // status == 1 [default]
  // settingsactive == 0 [default]

  if (status == 1)
  {
    if(settingsactive == 0){
      eeprom_val = EEPROMReadlong(0);
      if(int(eeprom_val) == 0 || isnan(int(eeprom_val)) || int(eeprom_val) < 0 || int(eeprom_val) > 36000){
        feed();
      } else {
        feedtime = eeprom_val;
        feed();
      }
    }
  } else {
    Blynk.run();
    settingsactive = 1;
    lcd.clear();

    // Leer valor potenciómetro
    selectedTime = analogRead(pot);
    selectedTime = map(selectedTime, 0, 1023, 10, 0);

    // Mostrar valor pontenciómetro en LCD
    lcd.setCursor(0, 0);
    lcd.print("Food Time:");
    lcd.setCursor(0, 1);
    lcd.print(String(selectedTime) + " hour");

    delay(200);
    // delay(1000);

    if (digitalRead(reset) == LOW) {
      feedtime = selectedTime * multiplier;

      EEPROMWritelong(0, feedtime); // mi prueba código hardcoded
      EEPROM.commit();

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Time Saved!");
      lcd.setCursor(0, 1);
      lcd.print(String(selectedTime) + " hour set");

      delay(2000);
      status = 1;
      settingsactive = 0;
    }
  }
}

BLYNK_WRITE(V1) {
  bylnkbtn = param.asInt();
  if (bylnkbtn == 1) {
    servo();
  }
}
BLYNK_WRITE(V5) {
  blynktimevalue = param.asInt();
}
BLYNK_WRITE(V6) {
  bylnkbtn = param.asInt();
  if (bylnkbtn == 1) {
    selectedTime = blynktimevalue;
    if(selectedTime == 0) {
      feedtime = defaultTime;
    } else {
      feedtime = selectedTime * multiplier;
    }
    EEPROMWritelong(0, feedtime);
    EEPROM.commit();
    status = 1;
  }
}
BLYNK_WRITE(V7) {
  restartbtn = param.asInt();
  if (restartbtn == 1) {
    ESP.restart();
  }
}

void feed()
{
  settingsactive = 0;

  while (feedtime > 0)
  {
    if(millis() >= currTime + checkPeriod){
      currTime += checkPeriod;
      CheckBlynk();
      eeprom();
    }

    second = feedtime;
    minute = second / 60;
    second = second % 60;
    hour = minute / 60;
    minute = minute % 60;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Feed time left");
    lcd.setCursor(0, 1);
    lcd.print(String(hour) + ":" + String(minute) + ":" + String(second));
    Blynk.virtualWrite(V2, String(hour) + ":" + String(minute) + ":" + String(second));

    delay(1000);
    feedtime--;

    // Después de restar 1 seg verifico si feedtime llegó a 0 por fin o no...
    if (feedtime == 0)
    {
      servo();
      if (selectedTime == 0) {
        feedtime = defaultTime;
      } else {
        feedtime = selectedTime * multiplier;
      }
      EEPROMWritelong(0, feedtime);
      EEPROM.commit();
    }

    if (digitalRead(reset) == 0) {
      while (digitalRead(reset) == LOW) {
        // delay(10); // No hace nada mientras el botón siga presionado
        yield();
      }
      status = 0;
      break;
    }
  }
}

#pragma region Código Servo Motor servo()

void servo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Enjoy your meal");
  motor.write(180);
  delay(2000);
  motor.write(0);
  delay(2000);
  Blynk.logEvent("feed_was_given", "The cat is happy!");
}

#pragma endregion