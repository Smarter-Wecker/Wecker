//LED
#define FASTLED_ALLOW_INTERRUPTS 0
#define FASTLED_ESP8266_RAW_PIN_ORDER
#include "FastLED.h"
//Display
#include "LiquidCrystal_I2C.h"
LiquidCrystal_I2C lcd(0x27, 16, 2);
//Zeit
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
BlynkTimer timer;
WidgetRTC rtc;
//Wetter
#include <ArduinoJson.h>                  
#include <math.h>                         
#include <WiFiManager.h>
WiFiManager wifiManager;
WiFiClient client;

const String city = "Oldenburg";
const String api_key = "";
char auth[] = "";
char ssid[] = "";
char pass[] = "";

int weatherID = 0;
int weatherID_shortened = 0;
String weatherforecast_shortened = " ";
int temperature = 0;

//Belegung
int buzzer = D5;
#define LED  D6
#define NUM_LEDS 4
CRGB leds[NUM_LEDS];
#define SENSOR D7

// 0 = kein Wecktag, 1 = Wecktag
int wochentage[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int weckstunde = 0;
int weckminute = 0;
String alarm_zeit;

bool wecktag_erreicht = false;
bool alarm_ton = false;
bool alarm = false;

// Time Lib Tag 1 = Sonntag = Blynk App Tag 7
int today () {
  int dayadjustment = -1;
  if (weekday() == 1) {
    dayadjustment = 6;
  }
  return weekday() + dayadjustment;
}

// Prüfe ob heute Wecktag ist
void check_wecktag() {
  if (wochentage[today()] == 1) {
    wecktag_erreicht = true;
  } else {
    wecktag_erreicht = false;
  }
}


void clockDisplay()
{
  String zeit = String(hour()) + ":" + minute() + ":" + second();
  String datum = String(day()) + " " + month() + " " + year();
  lcd.setCursor(0, 0);
  lcd.print(zeit);
  // Wenn für heute Wecker eingestellt wurde + Alarm Button in Blynk betätigt wurde..
  if(wecktag_erreicht && alarm) {
    // Alle 30 Sekunden wird die Weckzeit angezeigt für 10 Sekunden
    if(second() == 20 || second() == 50) {
    // Kästchen leeren
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(alarm_zeit);
    }
    // Ab jeder halben und ganzer Minute wird das Datum angezeigt für 20 Sekunden
    if(second() == 0 || second() == 30) {
      lcd.setCursor(0, 1);
      lcd.print("                ");
      lcd.setCursor(0, 1);
      lcd.print(datum);
    }
  } else {
    // Wenn kein Wecker eingestellt wurde, wird dauerhaft Datum angezeigt
    lcd.setCursor(0, 1);
    lcd.print("                ");
    lcd.setCursor(0, 1);
    lcd.print(datum);
  }
  
  if (alarm && wecktag_erreicht) {
    // Wenn die Weckzeit erreicht wurde, soll der Buzzer angehen
    if (weckstunde == hour() && weckminute == minute()) {
        alarm_ton = true;
    }
  } else {
    alarm_ton = false;
  }

  if (alarm_ton) {
    if(second() % 3 == 0) {
      noTone(buzzer);
    } else {
      tone(buzzer, 450);
    }
  } else {
    noTone(buzzer);
  }

  //Um Mitternacht prüfen, ob heute Wecktag ist
  if (hour() == 0 and minute() == 0) {
    check_wecktag();
  }
}


BLYNK_WRITE(V0)
{
  TimeInputParam t(param);

  // Für welche Tage wurde der Wecker eingestellt
  for (int i = 1; i <= 7; i++) {
    wochentage[i] = t.isWeekdaySelected(i);
  }
  
  weckstunde = t.getStartHour();
  weckminute = t.getStartMinute();
  // Zeit speichern
  alarm_zeit = "Alarm: " + String(weckstunde) + ":" + String(weckminute);
  check_wecktag();
}


BLYNK_WRITE(V1)
{
  int schalter = param.asInt();
  
  if (schalter == 1) {
    // Wenn man den Schalter in der Blynk App betätigt, hat man den Wecker aktiviert
    alarm = true;
  } else {
    // Wenn man den Schalter in der Blynk App nochmal betätigt, hat man den Wecker deaktiviert
    alarm = false;
  }
}


BLYNK_CONNECTED() {
  //Bei Verbindung Zeit synchronisieren
  rtc.begin();
}


void getCurrentWeatherConditions() {
  int WeatherData;
  if (client.connect("api.openweathermap.org", 80)) {
    client.println("GET /data/2.5/weather?q=" + city + ",DE&units=metric&lang=de&APPID=" + api_key);
    client.println("Host: api.openweathermap.org");
    client.println("Connection: close");
    client.println();
  } else {
    Serial.println("connection failed");
  }
  const size_t capacity = JSON_ARRAY_SIZE(2) + 2 * JSON_OBJECT_SIZE(1) + 2 * JSON_OBJECT_SIZE(2) + 2 * JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(14) + 360;
  DynamicJsonDocument doc(capacity);
  deserializeJson(doc, client);
  client.stop();

  // Wetter und Temperatur abfragen
  weatherID = doc["weather"][0]["id"];
  int temperature_Celsius = doc["main"]["temp"];
  temperature = (int)temperature_Celsius;
  weatherID_shortened = weatherID / 100;
  switch (weatherID_shortened) {
    //Durch die weatherID wird das Wetter bestimmt
    case 2: weatherforecast_shortened = "Sturm"; break;
    case 5: weatherforecast_shortened = "Regen"; break;
    case 6: weatherforecast_shortened = "Schnee"; break;
    case 8: weatherforecast_shortened = "Wolken"; break;
    default: weatherforecast_shortened = ""; break;
  } if (weatherID == 800) weatherforecast_shortened = "klar";
}


void clearSky() {
  lcd.setCursor(10, 0);
  lcd.print("Sonnig");
}

void cloudy() {
  lcd.setCursor(10, 0);
  lcd.print("Wolken");
}

void stormy() {
  lcd.setCursor(11, 0);
  lcd.print("Sturm");
}

void snowy() {
  lcd.setCursor(10, 0);
  lcd.print("Schnee");
}

void rainy() {
  lcd.setCursor(11, 0);
  lcd.print("Regen");
}


void setup() {
  pinMode(buzzer, OUTPUT);
  pinMode(SENSOR, INPUT);
  digitalWrite(buzzer, LOW);
  // Alle 10 Minuten wird die Zeit synchronisiert
  setSyncInterval(10 * 60);
  // Jede Sekunde Zeit prüfen
  timer.setInterval(1000L, clockDisplay);

  wifiManager.autoConnect("deineWetterLampe");
  delay(2000);
  getCurrentWeatherConditions();

  lcd.init();
  //Hintergrundbeleuchtung an
  lcd.backlight();
  
  delay(3000);
  Blynk.begin(auth, ssid, pass, "iot.informatik.uni-oldenburg.de", 8080);
  FastLED.addLeds<WS2811,LED,GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(50);
  FastLED.setMaxPowerInVoltsAndMilliamps(5,400);
  for (int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB( 0, 0, 0);
  }
  FastLED.show();
  delay(30);
  Serial.begin(115200);
}


void loop() {
  Blynk.run();
  timer.run();

  // Jede halbe Stunde wird das Wetter geprüft
  if(minute() == 30 || minute() == 0) {
    getCurrentWeatherConditions();
  }

  // Alle 30 Sekunden wird das Wetter angezeigt für 10 Sekunden
  // 1 Sekunde Verschiebung, damit es auf dem Display synchron angezeigt wird
  if(second() == 21 || second() == 51) {
    if (weatherID == 800){
      clearSky();
    } else {
    switch (weatherID_shortened) {
      case 2: stormy(); break;
      case 5: rainy(); break;
      case 6: snowy(); break;
      case 8: cloudy(); break;
      }
    }
  }
  
  // Ab jeder halben und ganzer Minute wird die Temperatur angezeigt für 20 Sekunden
  // 1 Sekunde Verschiebung, damit es auf dem Display synchron angezeigt wird
  if(second() == 1 || second() == 31) {
    // Die Kästchen leeren, da sonst weiterhin das Wetter angezeigt wird
    lcd.setCursor(10, 0);
    lcd.print("   ");
    
    // Temperatur anzeigen
    lcd.setCursor(13, 0);
    lcd.print(temperature);
    lcd.setCursor(15, 0);
    lcd.print("C");
  }

  // Da die Werte < 10 einstellig sind, bleibt nach der Sekundenzahl 59 die 9 weiterhin stehen und muss daher entfernt werden
  // Die Zahl wird bei der Sekundenzahl 1 entfernt, da es eine Verzögerung auf dem Display gibt, sodass es auf dem Display bei der Sekundenzahl 0 entfernt wird
  if(second() == 1 && minute() < 10 && hour() < 10) {
    lcd.setCursor(5, 0);
    lcd.print("  ");
  } else if(second() == 1 && minute() < 10 && hour() > 9) {
    lcd.setCursor(6, 0);
    lcd.print("  ");
  } else if(second() == 1 && minute() > 9 && hour() < 10) {
    lcd.setCursor(6, 0);
    lcd.print("  ");
  } else if(second() == 1 && minute() > 9 && hour() > 9) {
    lcd.setCursor(7, 0);
    lcd.print("  ");
  }
  
  // Falls der Bewegungssensor z.B. eine Handbewegung erkennt, wird Alarm und Buzzer deaktiviert
  if(digitalRead(SENSOR) && alarm_ton) {    
    Blynk.virtualWrite(V1, 0);
    alarm_ton = false;
  }

  // Wenn die Weckzeit erreicht wurde gibt es eine Animation 
  if(alarm_ton) {
    for (int i = 0; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    leds[1] = CRGB::Pink;
    leds[2] = CRGB::Pink;
    FastLED.setBrightness(120);
    FastLED.show();
    leds[0] = CRGB::Violet;
    leds[3] = CRGB::Violet;
    FastLED.setBrightness(140);
    FastLED.show();
  } else if(alarm && wecktag_erreicht && (hour() == weckstunde - 3 && weckminute - minute() > 30 || hour() == weckstunde - 4 && weckminute - minute() <= 0)) {
    // Wenn die Zeit bis zum Alarm weniger als 4 Stunden beträgt..
    // Ab jeder halben und ganzer Minute wird die Temperatur angezeigt für 20 Sekunden
    if(second() >= 0 && second() <= 20 || second() >= 30 && second() <= 50) {
      TemperaturAnzeige();
    } else {
      AlarmAnzeige();
    }
  } else {
    // Sonst wird dauerhaft die Temperatur angezeigt
    TemperaturAnzeige();
 }
}


void TemperaturAnzeige() {
  if(temperature >= 30) {
    // alle LED´s leuchten rot
    for (int i = 0; i < 4; i++) {
      leds[i] = CRGB::Red;
    }
    FastLED.show();
  } else if(temperature >= 25) {
    // 3 LED´s leuchten Orange und eine blinkt rot
    for (int i = 0; i < 3; i++) {
      leds[i] = CRGB::Orange;
    }
    if(second() % 2 == 0) {
      leds[3] = CRGB::Red;
      FastLED.show();
    } else {
      leds[3] = CRGB::Black;
      FastLED.show();
    }
  } else if(temperature >= 20) {
    // 2 LED´s leuchten Gelb und eine leuchtet Orange
    for (int i = 0; i < 2; i++) {
      leds[i] = CRGB::Yellow;
    }
    leds[2] = CRGB::Orange;
    leds[3] = CRGB::Black;
    FastLED.show();
  } else if(temperature >= 15) {
    // 2 LED´s leuchten Gelb
    for (int i = 0; i < 2; i++) {
      leds[i] = CRGB::Yellow;
    }
    for (int i = 2; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  } else if(temperature >= 10) {
    // 1 LED leuchtet Gelb
    leds[0] = CRGB::Yellow;
    for (int i = 1; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  } else if(temperature >= 5) {
    // 1 LED leuchtet Blau
    leds[0] = CRGB::Blue;
    for (int i = 1; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  } else if(temperature >= 0) {
    // 2 LED´s leuchten Blau
    for (int i = 0; i < 2; i++) {
      leds[i] = CRGB::Blue;
    }
    for (int i = 2; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
  } else if(temperature >= -5) {
    // 3 LED´s leuchtet Blau
    for (int i = 0; i < 3; i++) {
      leds[i] = CRGB::Blue;
    }
    leds[4] = CRGB::Black;
    FastLED.show();
  } else if(temperature >= -10) {
    // Alle LED´s leuchten Blau
    for (int i = 0; i < 4; i++) {
      leds[i] = CRGB::Blue;
    }
    FastLED.show();
  }
}


void AlarmAnzeige() {
  if(hour() == weckstunde && weckminute - minute() <= 30 || hour() == weckstunde - 1 && 60 - minute() + weckminute <= 30) {
    // < 0,5 Stunden bis zum Alarm (z.B. Zeit: 9:00 & Alarm: 9:30 oder Zeit: 8:45 & Alarm: 9:15)
    // 1 LED blinkt
    for (int i = 1; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    FastLED.show();
    if(second() % 2 == 0) {
      leds[0] = CRGB::Green;
      FastLED.show();
    } else {
      leds[0] = CRGB::Black;
      FastLED.show();
    }
  } else if(hour() == weckstunde && weckminute - minute() > 30 || hour() == weckstunde - 1 && weckminute - minute() <= 0) {
    // < 1 Stunde bis zum Alarm
    // 1 LED leuchtet
    for (int i = 1; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    leds[0] = CRGB::Green;
    FastLED.show();
  } else if(hour() == weckstunde - 1 && weckminute - minute() <= 30 || hour() == weckstunde - 2 && 60 - minute() + weckminute <= 30) {
    // < 1,5 Stunden bis zum Alarm
    // 1 LED leuchtet und 1 LED blinkt
    for (int i = 2; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    leds[0] = CRGB::Green;
    FastLED.show();
    if(second() % 2 == 0) {
      leds[1] = CRGB::Green;
      FastLED.show();
    } else {
      leds[1] = CRGB::Black;
      FastLED.show();
    }
  } else if(hour() == weckstunde - 1 && weckminute - minute() > 30 || hour() == weckstunde - 2 && weckminute - minute() <= 0) {
    // < 2 Stunden bis zum Alarm
    // 2 LED´s leuchten
    for (int i = 2; i < 4; i++) {
      leds[i] = CRGB::Black;
    }
    for (int i = 0; i < 2; i++) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
  } else if(hour() == weckstunde - 2 && weckminute - minute() <= 30 || hour() == weckstunde - 3 && 60 - minute() + weckminute <= 30) {
    // < 2,5 Stunden bis zum Alarm
    // 2 LED´s leuchten und 1 LED blinkt
    leds[3] = CRGB::Black;
    for (int i = 0; i < 2; i++) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
    if(second() % 2 == 0) {
      leds[2] = CRGB::Green;
      FastLED.show();
    } else {
      leds[2] = CRGB::Black;
      FastLED.show();
    }
  } else if(hour() == weckstunde - 2 && weckminute - minute() > 30 || hour() == weckstunde - 3 && weckminute - minute() <= 0) {
    // < 3 Stunden bis zum Alarm
    // 3 LED´s leuchten
    leds[3] = CRGB::Black;
    for (int i = 0; i < 3; i++) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
  } else if(hour() == weckstunde - 3 && weckminute - minute() <= 30 || hour() == weckstunde - 4 && 60 - minute() + weckminute <= 30) {
    // < 3,5 Stunden bis zum Alarm
    // 3 LED´s leuchten und 1 LED blinkt
    for (int i = 0; i < 3; i++) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
    if(second() % 2 == 0) {
      leds[3] = CRGB::Green;
      FastLED.show();
    } else {
      leds[3] = CRGB::Black;
      FastLED.show();
    }
  } else {
    // < 4 Stunden bis zum Alarm
    // 4 LED´s leuchten
    for (int i = 0; i < 4; i++) {
      leds[i] = CRGB::Green;
    }
    FastLED.show();
  }
}
