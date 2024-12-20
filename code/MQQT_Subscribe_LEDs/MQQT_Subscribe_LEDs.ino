// dodajemy potrzebne bilbioteki
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// dane WiFi
const char* ssid = "Michal Palant";     // SSID naszej sieci
const char* password = "kochamale321";  // hasło naszej sieci

// dane do połączenia z brokerem MQTT
const char* mqttServer = "192.168.71.45";  // adres IP brokera Mosquitto
const int mqttPort = 1883;                 // domyślny port MQTT
const char* mqttUser = "";                 // jeśli nie ma loginu - zostaw puste
const char* mqttPassword = "";             // jeśli nie ma hasła - zostaw puste

// ustawienia diody LED
const int redLED = D1;  // pin dla diody LED
const int greenLED = D2;
const int blueLED = D3;

// umozliwienie polaczenia z WiFi, a nastepnie z brokerem MQTT
WiFiClient espClient;
PubSubClient client(espClient);

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

  int ledPin = 0; // musimy uzywac strcmp, bo topic jest wskazniiem na chary
  if (strcmp(topic, "led/red/status") == 0) { ledPin = redLED; }
  else if (strcmp(topic, "led/green/status") == 0) { ledPin = greenLED; }
  else if (strcmp(topic, "led/blue/status") == 0) { ledPin = blueLED; }

  // funkcja do oblsugi właczania/wyłączania ledow
  MQTTSwitch(msg, ledPin);
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
}

  void loop() {
    // jesli nie jestesmy polaczeni z MQTT brokrem to staramy sie polaczyc
    if (!client.connected()) {
      reconnect();
    }

    // komenda obslugujaca przychodzace wiadomosci MQTT
    client.loop();
  }
