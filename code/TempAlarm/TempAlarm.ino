
// dodajemy potrzebne bilbioteki
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// dane WiFi
const char* ssid = "NETWORK";     // SSID naszej sieci
const char* password = "987654321";  // hasło naszej sieci

// dane do połączenia z brokerem MQTT
const char* mqttServer = "192.168.59.165";  // adres IP brokera Mosquitto
const int mqttPort = 1883;                 // domyślny port MQTT
const char* mqttUser = "";                 // jeśli nie ma loginu - zostaw puste
const char* mqttPassword = "";           // jeśli nie ma hasła - zostaw puste


// ustawienia diody LED
const int redLED = D1;  // pin dla diody LED
const int greenLED = D2;
const int blueLED = D3;
const int lm35Pin = A0;  // Pin analogowy
const int motionSensor = D6;
const int buzzer = D7;

float temperature = 0.0;
unsigned long lastMsg = 0;
char msg[50];

bool motionSensorOn = false;

// umozliwienie polaczenia z WiFi, a nastepnie z brokerem MQTT
WiFiClient espClient;
PubSubClient client(espClient);
// alarm dziala na przerwaniach - to sie wykona dopiero kiedy sensor wykryje ruch, jeśli alarm jest włączony
void IRAM_ATTR motionDetectedISR() {
  if(motionSensorOn){
    digitalWrite(buzzer, HIGH);
    Serial.println("Motion detected!");
    client.publish("sensor/movement", "Motion detected!");
  }
}

void setup() {
  // rozpoczecie komunikacji przez port szeregowy
  Serial.begin(115200);
  delay(10);

  // próba połączenia z wifi
  Serial.println();
  Serial.println("Łączenie z siecią WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Połączono z WiFi!");

  // łączenie z brokerem MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // ustawiamy pin didoy LED
  pinMode(redLED, OUTPUT);
  digitalWrite(redLED, LOW);  // dioda zawsze wylaczona na poczatku

  pinMode(greenLED, OUTPUT);
  digitalWrite(greenLED, LOW);  // dioda zawsze wylaczona na poczatku

  pinMode(blueLED, OUTPUT);
  digitalWrite(blueLED, LOW);  // dioda zawsze wylaczona na poczatku
  
  pinMode(lm35Pin, INPUT);

  pinMode(buzzer, OUTPUT);    //alarm
  digitalWrite(buzzer, LOW);
  pinMode(motionSensor, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(motionSensor), motionDetectedISR, RISING);
}

// funkcja zrobiona na modłę PubSubClient, odpowiadajaca za odczytywanie wiadomosci zasubskrybowanych tematów
void callback(char* topic, byte* payload, unsigned int length) 
{
  // legnth zwracane przy obsludze przez biblioteke PubSubClient, mowi jaka dlugosc bedzie miala wiadomosc
  char msg[length + 1];

  // odczytujemy wiadomosc zawarta w payload
  for (int i = 0; i < length; i++) 
  {
    msg[i] = (char)payload[i];
  }
  msg[length] = '\0';  // konczymy takim znakiem by program wiedzial ze string sie skonczyl (jest to wbudowane dla naszego jezyka programowania)

  // wyswietlamy jaki byl temat odebranej wiadomosci
  Serial.print("Wiadomość na temacie: ");
  Serial.println(topic);

  // no i co zawierala
  Serial.print("Odebrana wiadomość: ");
  Serial.println(msg);

  int pin = 0; // musimy uzywac strcmp, bo topic jest wskazniiem na chary
  if (strcmp(topic, "led/red/status") == 0) { pin = redLED; }
  else if (strcmp(topic, "led/green/status") == 0) { pin = greenLED; }
  else if (strcmp(topic, "led/blue/status") == 0) { pin = blueLED; }
  else if (strcmp(topic, "sensor/motion/status") == 0) {pin = motionSensor; }

  // funkcja do oblsugi właczania/wyłączania ledow
  MQTTSwitch(msg, pin);
}



// funkcja do polaczenia z MQTT brokerem
void reconnect() 
  {
  // proba polaczenia z MQTT Brokerem
  while (!client.connected()) 
  {
    Serial.print("Łączenie z brokerem MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) 
    {
      Serial.println("Połączono!");

      // subskrybowanie interesujacych nas tematow
      client.subscribe("led/red/status");
      client.subscribe("led/green/status");
      client.subscribe("led/blue/status");
      client.subscribe("sensor/motion/status");
    } 
    else 
    {
      Serial.print("Błąd, próba ponownego połączenia za 5 sekund...");
      delay(5000);
    }
  }
}

  // funkcja do obsługi switchów przez wiadomości MQTT, msg - wiadomość, pinNumber - numer pinu
void MQTTSwitch (char* msg, int pinNumber)
{
  // przy wiadomosci ON - wlacz diode
  if (strcmp(msg, "ON") == 0)         // strcmp - string compare, jesli takie same zwraca 0
  {
    digitalWrite(pinNumber, HIGH);  
  }

  // przy wiadomosci OFF - wylacz diode
  else if (strcmp(msg, "OFF") == 0) 
  {
    digitalWrite(pinNumber, LOW); 
  }

  if(strcmp(msg, "alarmON") == 0){
    motionSensorOn = true;
  }else if(strcmp(msg, "alarmOFF") == 0){
    motionSensorOn = false;
    digitalWrite(buzzer, LOW);
    client.publish("sensor/movement", "Alarm Off");
  }
}



void loop() {
  // jesli nie jestesmy polaczeni z MQTT brokrem to staramy sie polaczyc
  if (!client.connected()) {
    reconnect();
  }

  // komenda obslugujaca przychodzace wiadomosci MQTT
  client.loop();
  // to wysyla temperature co 5s
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;

    int analogValue = analogRead(lm35Pin);
    temperature = analogValue * (3.3 / 1024.0) * 100.0;  // Przeliczenie na °C

    Serial.printf("Temperatura: %.2f°C\n", temperature);

    // Wysyłanie przez MQTT
    snprintf(msg, sizeof(msg), "%.2f", temperature);
    client.publish("sensor/temperature", msg);
  }
  
}
