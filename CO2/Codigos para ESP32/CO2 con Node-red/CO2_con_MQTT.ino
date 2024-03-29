#include <WiFi.h>
#include <Arduino.h>
#include <PubSubClient.h>
#include <AverageValue.h>
#include <ArduinoJson.h>

//**************************************
//*********** MQ135 ***************
//**************************************
int MQ135Pin = 34;
int Rload = 1000;
double ppm = 419.47;
float a = 110.7432567;
float b = -2.856935538;
float suma_valores = 0;
float promedio_valores = 0;
const long MAX_VALUES_NUM = 10;
AverageValue<long> averageValue(MAX_VALUES_NUM);

//**************************************
//*********** WIFICONFIG ***************
//**************************************

const char* ssid = "Totalplay-559F";
const char* password = "559FF198UfvsqdDs";

//**************************************
//*********** MQTT CONFIG **************
//**************************************
const char* server = "broker.hivemq.com";
int port = 1883;

//**************************************
//*********** GLOBALES   ***************
//**************************************

WiFiClient esp32Client;
PubSubClient mqttClient(esp32Client);
int var = 0;
char datos[100];

//************************
//**   CONEXION WIFI  ****
//************************
void setup_wifi() {
  Serial.print("Conectándose a ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("Conectado a WiFi");
  Serial.println("Dirección IP: ");
  Serial.println(WiFi.localIP());
}

//************************
//** C A L L B A C K  ***
//************************

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");

  char payload_string[length + 1];

  int resultI;

  memcpy(payload_string, payload, length);
  payload_string[length] = '\0';
  resultI = atoi(payload_string);

  var = resultI;

  String resultS = "";

  for (int i = 0; i < length; i++) {
    resultS = resultS + (char)payload[i];
  }
  Serial.println();
}

//*****************************
//***    CONEXION MQTT      ***
//*****************************

void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Intentando conectarse MQTT...");
    // Creamos un cliente ID
    String clientId = "Tzec-Intento_";
    clientId += String(random(0xffff), HEX);

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Conectado");

      mqttClient.subscribe("CloudCo2/datos_mq135");

    } else {
      Serial.print("Fallo, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" intentar de nuevo en 5 segundos");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(10);
  setup_wifi();
  mqttClient.setServer(server, port);
  mqttClient.setCallback(callback);
  rutina();
}

void loop() {
  if (!mqttClient.connected()) {
    reconnect();
  }

  mqttClient.loop();

  // Leer valores del sensor
  float sensorPin = analogRead(MQ135Pin); // Pin del sensor MQ135
  float rs = (4095 * (Rload / sensorPin)) - Rload; // Medición de Rs del sensor
  float rSrO = rs / promedio_valores;
  float ppm = pow((rSrO / a), (1 / b));
  averageValue.push(ppm);
  Serial.print("ppm= ");
  Serial.println(averageValue.average());
  long ppm2 = averageValue.average();

  Serial.println(ppm2);
  delay(5000);

  // Crear el objeto JSON
  DynamicJsonDocument jsonDoc(100);
  jsonDoc["nombre"] = "esp32";
  jsonDoc["datos"] = ppm2;

  // Serializar el JSON a una cadena
  String jsonString;
  serializeJson(jsonDoc, jsonString);

  jsonString.toCharArray(datos, 100);

  mqttClient.publish("testeo/sensormq135-Tzec", datos);
  delay(5000);
}

//************************
//**   R U T I N A    ****
//************************
void rutina() {
  for (int x = 0; x < 10; x++) {
    float sensorPin = analogRead(MQ135Pin); // Pin del sensor MQ135
    float rs1 = (4095 * (Rload / sensorPin)) - Rload; // Medición de Rs del sensor
    float Ro = rs1 / (a * pow(ppm, b)); // R0 que se convierte en constante

    suma_valores = Ro + suma_valores;

    Serial.print("valor de rs = ");
    Serial.println(rs1);
    Serial.print("La suma de Ro es = ");
    Serial.println(Ro);
    delay(1000);
  }
  promedio_valores = suma_valores / 10;
  Serial.print("El promedio es = ");
  Serial.println(promedio_valores);
}
