// dodajemy potrzebne bilbioteki
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

// dane WiFi
const char* ssid = "Michal Palant";        // SSID naszej sieci
const char* password = "kochamale321"; // hasło naszej sieci

// dane do połączenia z brokerem MQTT
const char* mqttServer = "192.168.71.45";  // adres IP brokera Mosquitto 
const int mqttPort = 1883;                // domyślny port MQTT
const char* mqttUser = "";        // jeśli nie ma loginu - zostaw puste
const char* mqttPassword = "";    // jeśli nie ma hasła - zostaw puste

// ustawienia diody LED
const int ledPin = D1;  // pin dla diody LED

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

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("Połączono z WiFi!");

  // łączenie z brokerem MQTT
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  // ustawiamy pin didoy LED
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // dioda zawsze wylaczona na poczatku
}

// funkcja zrobiona na modłę PubSubClient, odpowiadajaca za odczytywanie wiadomosci zasubskrybowanych tematów
void callback(char* topic, byte* payload, unsigned int length) {
  // legnth zwracane przy obsludze przez biblioteke PubSubClient, mowi jaka dlugosc bedzie miala wiadomosc
  char msg[length + 1];

// odczytujemy wiadomosc zawarta w payload
  for (int i = 0; i < length; i++) 
  {
    msg[i] = (char)payload[i];
  }
  msg[length] = '\0'; // konczymy takim znakiem by program wiedzial ze string sie skonczyl (jest to wbudowane dla naszego jezyka programowania)

// wyswietlamy jaki byl temat odebranej wiadomosci
  Serial.print("Wiadomość na temacie: ");
  Serial.println(topic);

// no i co zawierala
  Serial.print("Odebrana wiadomość: ");
  Serial.println(msg);

  // przy wiadomosci ON - wlacz diode
  if (strcmp(msg, "ON") == 0)         // strcmp - string compare, jesli takie same zwraca 0
  {
    digitalWrite(ledPin, HIGH);  
  }

  // przy wiadomosci OFF - wylacz diode
  else if (strcmp(msg, "OFF") == 0) 
  {
    digitalWrite(ledPin, LOW); 
}


// funkcja do polaczenia z MQTT brokerem
void reconnect() 
{  
// proba polaczenia z MQTT Brokerem
  while (!client.connected()) {
    Serial.print("Łączenie z brokerem MQTT...");
    if (client.connect("ESP8266Client", mqttUser, mqttPassword)) 
    {
      Serial.println("Połączono!");
      
      // subskrybowanie interesujacych nas tematow
      client.subscribe("led/status");
    } else 
    {
      Serial.print("Błąd, próba ponownego połączenia za 5 sekund...");
      delay(5000);
    }
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
