#include <Arduino.h>
#include <pubsubclient.h>
#include <WiFi.h>
#include <Wire.h>
#include <MPU6050.h>
#include "DFRobot_BloodOxygen_S.h"

// WiFi settings
const char *ssid = "RouterIotFIA4";
const char *password = "RouterIotFIA4!";

// MQTT Broker settings
const char *mqtt_broker = "192.168.0.227"; // EMQX broker endpoint
const char *mqtt_topic1 = "dataCastres/topic1";
const char *mqtt_topic2 = "dataCastres/topic2";
const char *mqtt_topic3 = "homeTrainerCastres/Group2-A/MAC";
const char *mqtt_topic4 = "homeTrainerCastres/Group2-A/OIIA";
const char *mqtt_topic5 = "homeTrainerCastres/Group2-A/Coeur";
const char *mqtt_topic6 = "homeTrainerCastres/Group2-A/Bouton";
const char *mqtt_topic7 = "homeTrainerCastres/Group2-A/AverageBPM";
const char *mqtt_topic8 = "homeTrainerCastres/Group2-A/stopMeasure";

const char *mqtt_topic_angle = "homeTrainerCastres3/angle";
const char *mqtt_topic_spo2  = "hometrainer/spo2";
const char *mqtt_topic_bpm   = "hometrainer/bpm";
const char *mqtt_topic_temp  = "hometrainer/temp";
const int mqtt_port = 1883;

String client_id = "ArduinoClient-";

static unsigned long lastPublishTime = 0;
static unsigned long lastHeartRateTime = 0;

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// Accéléromètre
MPU6050 mpu;

#define I2C_ADDRESS 0x57
DFRobot_BloodOxygen_S_I2C MAX30102(&Wire, I2C_ADDRESS);

// ── Gyroscope ──────────────────────────────────────────────────
float angleX = 0, angleY = 0, angleZ = 0;
float gyroOffsetX = 0, gyroOffsetY = 0, gyroOffsetZ = 0;
float angleZeroX = 0; // position neutre du guidon
unsigned long lastGyroTime = 0;

// Heartbeat sensor
int ledPin = 13;
int analogPin = 0;
const int delayMsec = 60;
int averageBPM = 0;
bool averageBPMreceived = false;

// Bouton
int buttonPin = 10;
bool measureStarted = false;

int buttonState;            // the current reading from the input pin
int lastButtonState = HIGH;  // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

// Buzzer
#define D0 -1
#define D1 262
#define D2 293
#define D3 329
#define D4 349
#define D5 392
#define D6 440
#define D7 494
#define M1 523
#define M2 586
#define M3 658
#define M4 697
#define M5 783
#define M6 879
#define M7 987
#define H1 1045
#define H2 1171
#define H3 1316
#define H4 1393
#define H5 1563
#define H6 1755
#define H7 1971
#define WHOLE 1
#define HALF 0.5
#define QUARTER 0.25
#define EIGHTH 0.25
#define SIXTEENTH 0.625

// L'hymne à la joie
int tune[] =
{
  M3,M3,M4,M5, 
  M5,M4,M3,M2, 
  M1,M1,M2,M3, 
  M3,M2,M2, 
  M3,M3,M4,M5, 
  M5,M4,M3,M2, 
  M1,M1,M2,M3, 
  M2,M1,M1, 
  M2,M2,M3,M1, 
  M2,M3,M4,M3,M1, 
  M2,M3,M4,M3,M2, 
  M1,M2,D5,D0, 
  M3,M3,M4,M5, 
  M5,M4,M3,M4,M2, 
  M1,M1,M2,M3, 
  M2,M1,M1
};

float durt[]=
{
  1,1,1,1,
  1,1,1,1,
  1,1,1,1,
  1+0.5,0.5,1+1,
  1,1,1,1,
  1,1,1,1,
  1,1,1,1,
  1+0.5,0.5,1+1,
  1,1,1,1,
  1,0.5,0.5,1,1,
  1,0.5,0.5,1,1,
  1,1,1,1,
  1,1,1,1,
  1,1,1,0.5,0.5,
  1,1,1,1,
  1+0.5,0.5,1+1,
};

// Au clair de la Lune
int lune[] =
{
  M1,M1,M1,M2,M3,M2,
  M1,M3,M2,M2,M1
};

float rythmLune[] =
{
  1,1,1,1,1+1,1+1,
  1,1,1,1,1,
};

// int length;
int tonepin = 7;

// Zelda chest opening sound
/* This code is derived from:
 * http://www.arduino.cc/en/Tutorial/Melody
 * This plays the chest noise from the Legend of Zelda on a piezo buzzer connected to pin 9 and ground. It has been tuned to a buzzer I had on hand, but you may want to adjust the values, testing against a tuner.
 */
  
int speakerPin = 7;

char notes[] = "gabygabyxzCDxzCDabywabywzCDEzCDEbywFCDEqywFGDEqi        azbC"; // a space represents a rest
int length = sizeof(notes); // the number of notes
int beats[] = { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, 2,3,3,16,};
int tempo = 75;

// Star Wars - Imperial march
const int c = 261;
const int d = 294;
const int e = 329;
const int f = 349;
const int g = 391;
const int gS = 415;
const int a = 440;
const int aS = 455;
const int b = 466;
const int cH = 523;
const int cSH = 554;
const int dH = 587;
const int dSH = 622;
const int eH = 659;
const int fH = 698;
const int fSH = 740;
const int gH = 784;
const int gSH = 830;
const int aH = 880;
 
const int buzzerPin = 7;
const int ledPin1 = 12;
const int ledPin2   = 13;

// Star Wars - Force theme
/* 
  Star Wars theme  
  Connect a piezo buzzer or speaker to pin 11 or select a new pin.
  More songs available at https://github.com/robsoncouto/arduino-songs                                            
                                              
                                              Robson Couto, 2019
*/

#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

// change this to make the song slower or faster
int tempoForce = 108;

// change this to whichever pin you want to use
int buzzer = 7;

// notes of the moledy followed by the duration.
// a 4 means a quarter note, 8 an eighteenth , 16 sixteenth, so on
// !!negative numbers are used to represent dotted notes,
// so -4 means a dotted quarter note, that is, a quarter plus an eighteenth!!
int melody[] = {
  
  // Star Wars Main Theme 
  
  NOTE_AS4,8, NOTE_AS4,8, NOTE_AS4,8,//1
  NOTE_F5,2, NOTE_C6,2,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,  
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,  
  NOTE_AS5,8, NOTE_A5,8, NOTE_AS5,8, NOTE_G5,2, NOTE_C5,8, NOTE_C5,8, NOTE_C5,8,
  NOTE_F5,2, NOTE_C6,2,
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4,  
  
  NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F6,2, NOTE_C6,4, //8  
  NOTE_AS5,8, NOTE_A5,8, NOTE_AS5,8, NOTE_G5,2, NOTE_C5,-8, NOTE_C5,16, 
  NOTE_D5,-4, NOTE_D5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_F5,8, NOTE_G5,8, NOTE_A5,8, NOTE_G5,4, NOTE_D5,8, NOTE_E5,4,NOTE_C5,-8, NOTE_C5,16,
  NOTE_D5,-4, NOTE_D5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  
  NOTE_C6,-8, NOTE_G5,16, NOTE_G5,2, REST,8, NOTE_C5,8,//13
  NOTE_D5,-4, NOTE_D5,8, NOTE_AS5,8, NOTE_A5,8, NOTE_G5,8, NOTE_F5,8,
  NOTE_F5,8, NOTE_G5,8, NOTE_A5,8, NOTE_G5,4, NOTE_D5,8, NOTE_E5,4,NOTE_C6,-8, NOTE_C6,16,
  NOTE_F6,4, NOTE_DS6,8, NOTE_CS6,4, NOTE_C6,8, NOTE_AS5,4, NOTE_GS5,8, NOTE_G5,4, NOTE_F5,8,
  NOTE_C6,1
  
};

// sizeof gives the number of bytes, each int value is composed of two bytes (16 bits)
// there are two values per note (pitch and duration), so for each note there are four bytes
int notesForce = sizeof(melody) / sizeof(melody[0]) / 2;

// this calculates the duration of a whole note in ms
int wholenote = (60000 * 4) / tempoForce;

int divider = 0, noteDuration = 0;

// ------------------------------------------------------------
// STAR WARS FORCE THEME
// ------------------------------------------------------------
void playForceTheme()
{
  // iterate over the notes of the melody. 
  // Remember, the array is twice the number of notes (notes + durations)
  for (int thisNote = 0; thisNote < notesForce * 2; thisNote = thisNote + 2) {

    // calculates the duration of each note
    divider = melody[thisNote + 1];
    if (divider > 0) {
      // regular note, just proceed
      noteDuration = (wholenote) / divider;
    } else if (divider < 0) {
      // dotted notes are represented with negative durations!!
      noteDuration = (wholenote) / abs(divider);
      noteDuration *= 1.5; // increases the duration in half for dotted notes
    }

    // we only play the note for 90% of the duration, leaving 10% as a pause
    tone(buzzer, melody[thisNote], noteDuration*0.9);

    // Wait for the specief duration before playing the next note.
    delay(noteDuration);
    
    // stop the waveform generation before the next note.
    noTone(buzzer);
  }
}

// Function declarations
void connectToWiFi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);

// ------------------------------------------------------------
// ZELDA MUSIC
// ------------------------------------------------------------
void playTone(int tone, int duration) {
  for (long i = 0; i < duration * 1000L; i += tone * 2) {
    digitalWrite(speakerPin, HIGH);
    delayMicroseconds(tone);
    digitalWrite(speakerPin, LOW);
    delayMicroseconds(tone);
  }
}

void playNote(char note, int duration) {
  char names[] = { 'c', 'd', 'e', 'f', 'g', 'x', 'a', 'z', 'b', 'C', 'y', 'D', 'w', 'E', 'F', 'q', 'G', 'i' };
  // c=C4, C = C5. These values have been tuned.
  int tones[] = { 1898, 1690, 1500, 1420, 1265, 1194, 1126, 1063, 1001, 947, 893, 843, 795, 749, 710, 668, 630, 594 };
   
  // play the tone corresponding to the note name
  for (int i = 0; i < 18; i++) {
    if (names[i] == note) {
      playTone(tones[i], duration);
    }
  }
}

void playZeldaMusic()
{
  // Zelda music
    pinMode(speakerPin, OUTPUT);
    for (int i = 0; i < length; i++)
    {
      if (notes[i] == ' ')
      {
        delay(beats[i] * tempo); // rest
      }
      else
      {
        playNote(notes[i], beats[i] * tempo);
      }
    
    delay(tempo / 2);
  }
}

// ------------------------------------------------------------
// STAR WARS MUSIC - Imperial March
// ------------------------------------------------------------
void beep(int note, int duration)
{
  //Play tone on buzzerPin
  tone(buzzerPin, note);

  delay(duration); // maintain the tone for 'duration' milliseconds
  //Stop   tone on buzzerPin
  noTone(buzzerPin);
 
  delay(duration * 0.30);
}
 
void firstSection()
{
  beep(a, 500);
   beep(a, 500);    
  beep(a, 500);
  beep(f, 350);
  beep(cH, 150);  
   beep(a, 500);
  beep(f, 350);
  beep(cH, 150);
  beep(a, 650);
 
   delay(500);
 
  beep(eH, 500);
  beep(eH, 500);
  beep(eH, 500);  
   beep(fH, 350);
  beep(cH, 150);
  beep(gS, 500);
  beep(f, 350);
   beep(cH, 150);
  beep(a, 650);
 
  delay(500);
}
 
void secondSection()
{
   beep(aH, 500);
  beep(a, 300);
  beep(a, 150);
  beep(aH, 500);
  beep(gSH,   325);
  beep(gH, 175);
  beep(fSH, 125);
  beep(fH, 125);    
  beep(fSH,   250);
 
  delay(325);
 
  beep(aS, 250);
  beep(dSH, 500);
  beep(dH,   325);  
  beep(cSH, 175);  
  beep(cH, 125);  
  beep(b, 125);  
  beep(cH,   250);  
 
  delay(350);
}

void playImperialMarch()
{
  // Star Wars music - Imperial march
  //Play first section
  firstSection();
   
  //Play second section
  secondSection();
 
  //Variant 1
  beep(f,   250);  
  beep(gS, 500);  
  beep(f, 350);  
  beep(a, 125);
  beep(cH,   500);
  beep(a, 375);  
  beep(cH, 125);
  beep(eH, 650);
 
  delay(500);
   
  //Repeat second section
  secondSection();
 
  //Variant 2
  beep(f,   250);  
  beep(gS, 500);  
  beep(f, 375);  
  beep(cH, 125);
  beep(a,   500);  
  beep(f, 375);  
  beep(cH, 125);
  beep(a, 650);  
 
  delay(650);
}

// ── Calibration gyroscope au démarrage ────────────────────────
void calibrateGyro() {
  Serial.println("Calibration gyroscope...");
  Serial.println("Gardez le capteur IMMOBILE pendant 3 secondes !");

  int samples = 300;
  long sumGX = 0, sumGY = 0, sumGZ = 0;

  for (int i = 0; i < samples; i++) {
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    sumGX += gx;
    sumGY += gy;
    sumGZ += gz;
    delay(10);
  }

  gyroOffsetX = sumGX / (float)samples;
  gyroOffsetY = sumGY / (float)samples;
  gyroOffsetZ = sumGZ / (float)samples;

  Serial.println("Calibration terminée !");
  Serial.print("Offset X: "); Serial.print(gyroOffsetX);
  Serial.print(" | Y: "); Serial.print(gyroOffsetY);
  Serial.print(" | Z: "); Serial.println(gyroOffsetZ);

  // Position neutre = position actuelle du guidon
  angleX = 0;
  angleY = 0;
  angleZ = 0;
  angleZeroX = 0;

  lastGyroTime = millis();
  Serial.println("Position neutre enregistrée — vous pouvez bouger !");
}

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup()
{
  Serial.begin(9600);
  // Accéléromètre
  Wire.begin();

  // MPU-6050
  mpu.initialize();
  if (mpu.testConnection()) {
    Serial.println("MPU-6050 connecté !");
  } else {
    Serial.println("Erreur MPU-6050 !");
  }

  // Calibration — guidon droit au démarrage !
  calibrateGyro();

  // MAX30102
  Serial.println("Initializing MAX30102...");
  while (false == MAX30102.begin()) {
    Serial.println("init fail! Check wiring / I2C mode / power");
    delay(1000);
  }
  Serial.println("MAX30102 init success!");
  MAX30102.sensorStartCollect();

  connectToWiFi();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  // Heartbeat sensor init
  pinMode(ledPin, OUTPUT);

  Serial.println("Heartbeat Detektion Beispielcode.");

  // L'hymne à la joie
  // length = sizeof(tune)/sizeof(tune[0]);

  // Star Wars music
  //Setup pin modes
  pinMode(buzzerPin, OUTPUT);

  Serial.println("MUSIC ZELDA");
  playZeldaMusic();

    // Au clair de la Lune
  // Change lune and rythmLune variables with tune and durt variables to play L'hymne à la joie
  // for(int x=0; x<length; x++)
  // {
  //   tone(tonepin, lune[x]);
  //   delay(500 * rythmLune[x]); noTone(tonepin);
  // }

  // delay(2000);

  delay(100);

  // Bouton
  pinMode(buttonPin, INPUT_PULLUP);
  digitalWrite(buttonPin, HIGH);
}

// ------------------------------------------------------------
// PRINT MAC ADDRESS
// ------------------------------------------------------------
String printMacAddress()
{
  String MAC_address = "";
  
  byte mac[6];
  Serial.print("MAC Address: ");

  WiFi.macAddress(mac);

  for (int i = 0; i < 6; i++)
  {
    MAC_address += String(mac[i], HEX);
    if (i < 5)
      MAC_address += ":";
    if (mac[i] < 16)
      client_id += "0";
    client_id += String(mac[i], HEX);
  }

  Serial.println(MAC_address);
  return MAC_address;
}

// ------------------------------------------------------------
// WIFI CONNECTION
// ------------------------------------------------------------
void connectToWiFi()
{
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  delay(3000);
  printMacAddress();

  Serial.println("Connected to the WiFi network");
  Serial.print("Local ESP32 IP: ");
  Serial.println(WiFi.localIP());
}

// ------------------------------------------------------------
// MQTT CONNECTION
// ------------------------------------------------------------
void connectToMQTTBroker()
{
  while (!mqtt_client.connected())
  {
    Serial.print("Connecting to MQTT Broker as ");
    Serial.print(client_id.c_str());
    Serial.println("...");

    if (mqtt_client.connect(client_id.c_str()))
    {
      Serial.println("Connected to MQTT broker");

      // mqtt_client.subscribe(mqtt_topic1);
      // mqtt_client.subscribe(mqtt_topic2);
      mqtt_client.subscribe(mqtt_topic3);
      mqtt_client.subscribe(mqtt_topic4);
      mqtt_client.subscribe(mqtt_topic5);
      mqtt_client.subscribe(mqtt_topic6);
      mqtt_client.subscribe(mqtt_topic7);
      mqtt_client.subscribe(mqtt_topic8);

      String message = "Hello EMQX I'm " + client_id;
      mqtt_client.publish(mqtt_topic1, message.c_str());
    }
    else
    {
      Serial.print("Failed, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

// ------------------------------------------------------------
// MQTT CALLBACK
// ------------------------------------------------------------
void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  Serial.print("Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    messageTemp += (char)payload[i];
  }

  Serial.println(messageTemp);
  Serial.println("-----------------------");

  // TODO : Add your message handling logic here
  // For example, you can check the topic and perform actions based on the message content
  // Example:
  if(String(topic) == String(mqtt_topic7))
  {
    Serial.println("______________AVERAGE BPM______________");
    averageBPM = messageTemp.toInt();
    averageBPMreceived = true;
  }

  if(String(topic) == String(mqtt_topic8) && messageTemp == "Stop measure")
  {
    Serial.println("__________STOP MEASURE BPM____________");
    measureStarted = false;
  }
}

// ------------------------------------------------------------
// HEARTBEAT SENSOR
// ------------------------------------------------------------
int rawValue;

bool heartbeatDetected(int IRSensorPin, int delay)
{
  static int maxValue = 0;
  static bool isPeak = false;

  bool result = false;

  rawValue = analogRead(IRSensorPin);
  rawValue *= (1000 / delay);

  if (rawValue * 4L < maxValue)
    maxValue = rawValue * 0.8;

  if (rawValue > maxValue - (1000 / delay))
  {

    if (rawValue > maxValue)
      maxValue = rawValue;

    if (!isPeak)
      result = true;

    isPeak = true;
  }
  else if (rawValue < maxValue - (3000 / delay))
  {
    isPeak = false;
    maxValue -= (1000 / delay);
  }

  return result;
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------
void loop()
{
  if (!mqtt_client.connected())
    connectToMQTTBroker();

  mqtt_client.loop();

  unsigned long currentTime = millis();

  // Accéléromètre
  unsigned long now = millis();
  if(now - lastPublishTime >= 100)
  {
    lastPublishTime = now;

    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float dt = (now - lastGyroTime) / 1000.0;
    lastGyroTime = now;

    // Soustrait l'offset de calibration
    float gxDeg = (gx - gyroOffsetX) / 131.0;
    float gyDeg = (gy - gyroOffsetY) / 131.0;
    float gzDeg = (gz - gyroOffsetZ) / 131.0;

    angleX += gxDeg * dt;
    angleY += gyDeg * dt;
    angleZ += gzDeg * dt;

    // Angle guidon : X centré sur 0 au démarrage → -30 à +30
    float angleGuidon = constrain(angleX, -30, 30);

    Serial.print("X: "); Serial.print(angleX, 1);
    Serial.print("° | Y: "); Serial.print(angleY, 1);
    Serial.print("° | Z: "); Serial.print(angleZ, 1);
    Serial.print("° | Guidon: "); Serial.print(angleGuidon*(-1), 1);
    Serial.println("°");

    mqtt_client.publish(mqtt_topic_angle, String(angleGuidon*(-1), 1).c_str());
  }

  // ── Heart rate + SPO2 toutes les 4 secondes ────────────────
  if (now - lastHeartRateTime >= 4000) {
    lastHeartRateTime = now;

    MAX30102.getHeartbeatSPO2();

    int spo2      = MAX30102._sHeartbeatSPO2.SPO2;
    int heartbeat = MAX30102._sHeartbeatSPO2.Heartbeat;
    float temp    = MAX30102.getTemperature_C();

    mqtt_client.publish(mqtt_topic_spo2, String(spo2).c_str());
    mqtt_client.publish(mqtt_topic_bpm,  String(heartbeat).c_str());
    mqtt_client.publish(mqtt_topic_temp, String(temp, 1).c_str());

    Serial.print("SPO2: ");       Serial.print(spo2);      Serial.println(" %");
    Serial.print("Heart Rate: "); Serial.print(heartbeat); Serial.println(" bpm");
    Serial.print("Temperature: ");Serial.print(temp);      Serial.println(" C");
    Serial.println("----------------------");
  }

  static int beatMsec = 0;
  int heartRateBPM = 0;

  // Quand on reçoit la moyenne : on joue une musique
  if(averageBPMreceived)
  {
    if(averageBPM > 100)
    {
      Serial.println("MUSIQUE IMPERIAL MARCH");
      playImperialMarch();
    }
    else
    {
      Serial.println("MUSIQUE THEME FORCE");
      playForceTheme();
    }

    averageBPMreceived = false;
  }

  // read the state of the switch into a local variable:
  int readingButton = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if(readingButton != lastButtonState)
  {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if((millis() - lastDebounceTime) > debounceDelay)
  {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if(readingButton != buttonState)
    {
      buttonState = readingButton;
    }
  }

  // save the readingButton. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = readingButton;

  // Bouton
  if(readingButton == LOW)
  {
    measureStarted = true;

    mqtt_client.publish(mqtt_topic6, "Début de prise de pouls");
    Serial.println("Average BPM : " + averageBPM);

    Serial.println("Bouton appuyé !");
    digitalWrite(buttonPin, HIGH);
  }

  // Heartbeat detection
  if(measureStarted)
  {
    if(heartbeatDetected(analogPin, delayMsec))
    {
      heartRateBPM = 60000 / beatMsec;

      if(heartRateBPM > 30 && heartRateBPM < 150)
      {
        // Light the LED when a heartbeat is detected
        digitalWrite(ledPin, HIGH);
        // Play a sound when a hearbeat is detected
        tone(buzzerPin, H7, 5);

        String msgCoeur = String(heartRateBPM);
        mqtt_client.publish(mqtt_topic5, msgCoeur.c_str());

        Serial.print("Puls erkannt: ");
        Serial.println(heartRateBPM);
      }

      beatMsec = 0;
    }
    else
    {
      digitalWrite(ledPin, LOW);
    }

    delay(delayMsec);
    beatMsec += delayMsec;
  }

  // Publish MAC periodically
  if (currentTime - lastPublishTime >= 10000)
  {
    lastPublishTime = currentTime;

    String MAC = printMacAddress();
    String MACmsg = "Adresse MAC " + MAC;

    String MAC_OIIA = "OIIA á“šá˜á—¢ MAC Address : " + printMacAddress();

    // Uncomment if needed
    // mqtt_client.publish(mqtt_topic3, MACmsg.c_str());
    // mqtt_client.publish(mqtt_topic4, "Miaou");
  }
}