#include <AsyncElegantOTA.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HTTPClient.h>

// WEBServer
AsyncWebServer server(80);

// Data wire is connected to GPIO 4
#define ONE_WIRE_BUS 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// Timer variables
unsigned long lastTime = 0;  
unsigned long timerDelay = 60000; // Delay de envio do valor de temperatura

const char WIFI_SSID[] = "Projeto MOVE"; // SSID da rede wifi
const char WIFI_PASSWORD[] = "!Ceasa@2023"; // Senha da rede wifi

String readDSTemperatureC() {
  // Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
  sensors.requestTemperatures(); 
  float tempC = sensors.getTempCByIndex(0);

  if(tempC == -127.00) {
    Serial.println("Failed to read from DS18B20 sensor");
    return "--";
  } else {
    Serial.print("Temperature Celsius: ");
    Serial.println(tempC); 
  }
  return String(tempC);
}

void post_values(String temp){
  HTTPClient http;
  
  // Dados para o Banco de Dados
  const String ID = "Test2"; // Identificação do sensor
  const int portaria = 3; // Número da portaria. Portaria 1, Portaria 2, Portaria 3...
  const String tpacesso = "S"; // E para entrada e S para saída
  const int acesso = 1; // Número do acesso da portaria. Portaria 2 e Acesso 1. O Acesso 1 pode ser o Acesso 1 da entrada ou o Acesso 1 da saída.

  // URL para onde serão enviados os dados
  String HOST_NAME = "http://10.5.62.152:8480";
  String PATH_NAME   = "/esp32/echo.php";
  String queryString = "?ID="+ID+"&ST="+temp+"&PORT="+portaria+"&TPACESS="+tpacesso+"&ACESS="+acesso; // Passando valores via GET
  Serial.println(HOST_NAME + PATH_NAME + queryString);
  http.begin(HOST_NAME + PATH_NAME + queryString); //HTTP
  int httpCode = http.GET();

  // httpCode will be negative on error
  if(httpCode > 0) {
    // file found at server
    if(httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.println(payload);
    } else {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
    }
  } else {
    Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}

void setup() {
  Serial.begin(115200); 

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  // OTA
  AsyncElegantOTA.begin(&server);
  server.begin();
  
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    String temp = readDSTemperatureC();
    post_values(temp);
    lastTime = millis();
  } 
  
}