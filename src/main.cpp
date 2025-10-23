#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define Do_pin 17
#define AO_pin 34

#define Screen_Width 128
#define Screen_Height 64
Adafruit_SSD1306 display(Screen_Width, Screen_Height, &Wire, -1);

const int dhtpin = 16; 
DHTesp dhtSensor;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

void wifi_setup() {
  delay(10);
  Serial.println("Connecting to Wifi");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(".");
  }

  Serial.println("");
  Serial.println("Connected");
}

void callbcak(char* topic, byte* payload, unsigned int length){
  Serial.print("Message Accepted at topic: ");
  Serial.print(String(topic) + "\n");
  Serial.print("Message: ");
  for(int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void reconnect(){
  while(!client.connected()){
    Serial.print("Connecting to MQTT ...");
    if(client.connect("WSP32Client")){
      Serial.println("Conneccted!");
      client.subscribe("smarthome/data");
    } else {
      Serial.print("Failed, rc = ");
      Serial.print(client.state());
      Serial.println(" Try to connect in 5 secon...");
      delay(5000);
    }
  }
}

void setup() {
  // put your setup code here, to run once:
  dhtSensor.setup(dhtpin, DHTesp::DHT22);
  Wire.begin(21, 22); 
  Serial.begin(115200);

  wifi_setup();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callbcak);

  pinMode(Do_pin, INPUT);
  analogSetAttenuation(ADC_11db);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c)){
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(2000);
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(5, 30);
  display.println("SmartHome Monitoring");
  display.display();
  Serial.println("Hello World!");
  delay(1000);
  // display.clearDisplay();
  for (int i = 0; i < 25 && WiFi.status() != WL_CONNECTED; i++){
    delay(300);
    display.setCursor(i+30, 41);
    display.println(".");
    display.display();
    // display.clearDisplay();
  }
  display.clearDisplay();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  int gassstate = digitalRead(Do_pin);
  int gasvalue = analogRead(AO_pin);

  String payload = "{";
  payload += "\"temperature\": " + String(data.temperature, 1) + ",";
  payload += "\"humidity\": " + String(data.humidity, 1) + ",";
  payload += "\"gas_value\": " + String(gasvalue) + ",";
  payload += "\"gas_state\": \"" + String((gassstate == HIGH) ? 0 : 1) + "\"";
  payload += "}";

  client.publish("smarthome/data", payload.c_str());
  Serial.println("Published JSON: " + payload);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Temperature : "+ String(data.temperature, 1) + " C");
  display.setCursor(0, 12);
  display.println("Humidity    : "+ String(data.humidity, 1) + " %");
  display.setCursor(0, 23);
  if (gassstate == HIGH) {
    display.println("Gas         : Low");
  } else {
    display.println("Gas         : High");
  }
  display.setCursor(0, 34);
  display.println("Sensor Value: " + String(gasvalue));
  display.display();
  delay(500);
}
