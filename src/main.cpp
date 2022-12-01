#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFiManager.h>
#include <HTTPClient.h>

// Implements TRIGGEN_PIN button press, press for ondemand configportal, hold for 3 seconds for reset settings.
#define TRIGGER_PIN 27
#define TEMPERATURE_PIN 5
#define RELAY_PIN 21
#define MAX_TEMP 60

// NC = Nova Ceasa, P1 = Portaria 1, C1 = Cancela 1
#define ID "NC_P1_C1" 

// wifimanager can run in a blocking mode or a non blocking mode
// Be sure to know how to process loops with no delay() if using non blocking
bool wm_nonblocking = false; // change to true to use non blocking

WiFiManager wm; // global wm instance

//Your Domain name with URL path or IP address with path
const char* serverName = "http://localhost/update-sensor.php";

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastTime = 0;
// Timer set to 1 minute (600000)
unsigned long timerDelay = 60000;
// Set timer to 5 seconds (5000)
// unsigned long timerDelay = 5000;

unsigned long relay_lastTime = 0;
unsigned long relay_timerDelay = 1000;

int temperatura;
OneWire oneWire(TEMPERATURE_PIN); // Cria um objeto OneWire
DallasTemperature sensor(&oneWire); // Informa a referencia da biblioteca dallas temperature para Biblioteca onewire
DeviceAddress endereco_temp; // Cria um endereco temporario da leitura do sensor

void setup() {
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP 
  Serial.begin(115200);
  wm.setDebugOutput(true);
  delay(3000);
  Serial.println("\n Starting");

  pinMode(TRIGGER_PIN, INPUT);

  // wm.resetSettings(); // wipe settings

  if(wm_nonblocking) wm.setConfigPortalBlocking(false);

  sensor.begin(); ; // Inicia o sensor
  pinMode(RELAY_PIN, OUTPUT); // Define o pino do relé como saída
}

void checkButton(){
  // check for button press
  if ( digitalRead(TRIGGER_PIN) == LOW ) {
    // poor mans debounce/press-hold, code not ideal for production
    delay(50);
    if( digitalRead(TRIGGER_PIN) == LOW ){
      Serial.println("Button Pressed");
      // still holding button for 3000 ms, reset settings, code not ideal for production
      delay(3000); // reset delay hold
      if( digitalRead(TRIGGER_PIN) == LOW ){
        Serial.println("Button Held");
        Serial.println("Erasing Config, restarting");
        wm.resetSettings();
        ESP.restart();
      }
      
      // start portal w delay
      Serial.println("Starting config portal");
      wm.setConfigPortalTimeout(120);
      
      if (!wm.startConfigPortal("Alarme de temperatura","1234")) {
        Serial.println("failed to connect or hit timeout");
        delay(3000);
        // ESP.restart();
      } else {
        //if you get here you have connected to the WiFi
        Serial.println("connected...yeey :)");
      }
    }
  }
}

int sensor_read(){
  sensor.requestTemperatures(); // Envia comando para realizar a conversão de temperatura
  if (!sensor.getAddress(endereco_temp,0)) { // Encontra o endereco do sensor no barramento
    Serial.println("SENSOR NAO CONECTADO"); // Sensor conectado, imprime mensagem de erro
    return -1;
  } else {
    Serial.print("Temperatura = "); // Imprime a temperatura no monitor serial
    Serial.println(sensor.getTempC(endereco_temp), 1); // Busca temperatura para dispositivo
    return sensor.getTempC(endereco_temp);
  }
}

void POST(String identificador, int temperatura){
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      WiFiClient client;
      HTTPClient http;
    
      // Your Domain name with URL path or IP address with path
      http.begin(client, serverName);
      
      // Specify content-type header
      http.addHeader("Content-Type", "text/plain");
      // Data to send with HTTP POST
      String httpRequestData = "ID: " + identificador + "Temperatura: " + temperatura;
      // Send HTTP POST request
      int httpResponseCode = http.POST(httpRequestData);
     
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
      

      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

void relay(int temperatura){
  if ((millis() - relay_lastTime) > relay_timerDelay) {
    if (temperatura >= MAX_TEMP){ // Caso temperatura ultrapasse os 30 graus Celsius aciona o rele
      digitalWrite(RELAY_PIN, LOW); // Rele acionado
    } 
    else  {
      digitalWrite(RELAY_PIN, HIGH); // Temperatura abaixo de 30 graus Celsius desliga o rele
    }
    relay_lastTime = millis();
  }
}

void loop() {
  if(wm_nonblocking) wm.process(); // avoid delays() in loop when non-blocking and other long running code  
  checkButton();
  temperatura = sensor_read();
  relay(temperatura);
  POST(ID, temperatura);
  
}