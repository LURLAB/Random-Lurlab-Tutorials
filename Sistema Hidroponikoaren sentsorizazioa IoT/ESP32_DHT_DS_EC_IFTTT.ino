/***********************************************************************************

LURLAB RURAL MAKERSPACE

Webgunea: https://www.lurlab.org/
GitHub: https://github.com/lurlab
Emaila: info@lurlab.org

Proiektua: 
Kodearen egilea: Eñaut Ezkurdia Muñoa
Data: 2022ko apirilaren 28a

***********************************************************************************/

#include <WiFi.h>   //WiFi-ra konektatzeko liburutegia

//Zure WiFi sarean izena eta pasahitza jarri kakotxen barruan
const char* ssid     = "WiFi sarearen izena";   //Aldatu lerro hau zure wifiaren izenarekin
const char* password = "pasahitza";   //Aldatu lerro hau zure wifiaren pasahitzarekin

//Aldatu lerro hau zure IFTTT-ko gertakizunaren izenarekin (event) eta giltza-zenbakiarekin (key) 
const char* resource = "https://maker.ifttt.com/trigger/EVENT_NAME/with/key/dhJdzqY3Hdikay5JfjMnm8";    //IFTTT - webhooks - documentation
const char* server = "maker.ifttt.com"; //Erabiliko den zerbitzaria IoT
uint64_t uS_TO_S_FACTOR = 1000000;    //Mikrosegunduetatik segunduetara aldatzeko
uint64_t TIME_TO_SLEEP = 500;   //Jarri lerro honetan zenbateko tartea nahi den datuak zenbat segunduro jaso nahi diren

#include <DHT.h>    //Airearen tenperatura eta hezetasuna lortzeko liburutegia
#define DHTPIN 13   //Mikrokontrolagailuan DHT11 sentsorea konektatzeko erabiliko den hanka
#define DHTTYPE DHT11   //Erabiliko dugun sentsore mota
DHT dht(DHTPIN, DHTTYPE);   //DHT11 sentsorearen konfigurazioa

//Uraren tenperatura lortzeko liburutegiak
#include <OneWire.h>    
#include <DallasTemperature.h>    
#define oneWireBus 33   //Mikrokontrolagailuan DS18B20 sentsorea konektatzeko erabiliko den hanka
//DS18B20 sentsorearen konfigurazioa
OneWire oneWire(oneWireBus);
DallasTemperature sensors(&oneWire);


#define TdsSensorPin 32 //Mikrokontrolagailuan EC Tds sentsorea konektatzeko erabiliko den hanka
#define VREF 3.3      // Mikrokontrolagailuaren AnalogicDigitalConverter (ADC) erreferentziako tentsioa
float averageVoltage = 0,tdsValue = 0,tdsValueA = 0, ZuzenketaKoef = 0.15; //Zuzenketa koefizientea beharren arabera egokitu

void setup()
{
    Serial.begin(115200);   //Datuen trasferentziarako abiadura
    dht.begin();    //DHT11 sentsorearen hasieraketa
    sensors.begin();    //DS18B20 sentsorearen hasieraketa
    pinMode(TdsSensorPin,INPUT);    //EC Tds sentsorearen konfigurazioa

      //Sentsoreen datuak lortu
      float t = dht.readTemperature();    //airearen tenperatura
      //float h = dht.readHumidity();   //airearen hezetasuna
      sensors.requestTemperatures(); 
      float temperatureC = sensors.getTempCByIndex(0);    //uraren tenperatura
      
      int ADCval = analogRead(TdsSensorPin);    //EC Tds sentsoretik jasotako tentsioaren irakurketa ADC
      averageVoltage =  (ADCval* (float)VREF / 4095.0) - ZuzenketaKoef; // ADC baliotik tentsioaren baliora pasatzeko eragiketa
      float compensationCoefficient=1.0+0.02*(t-25.0);    //tenperaturaren araberako moldaketa
      float compensationVolatge=averageVoltage/compensationCoefficient;  //tenperaturaren araberako moldaketa
      //Tentsioaren baliotik EC (ppm) baliora pasatzeko eragiketa
      tdsValue=(133.42*compensationVolatge*compensationVolatge*compensationVolatge - 255.86*compensationVolatge*compensationVolatge + 857.39*compensationVolatge)*0.5; //convert voltage value to tds value 

          //Balioak IFTTT plataformara bidaltzeko prestatu
          float value1 = t;
          float value2 = temperatureC;
          float value3 = tdsValue;

        //Serie monitorean sentsoreetatik jasotako balioak erakutsi
        Serial.print("Temperatura (Airea): ");
        Serial.print(value1);
        Serial.print("ºC Tenperatura (Ura): ");
        Serial.print(value2);
        Serial.print("ºC Eroankortasuna: ");
        Serial.print(value3);
        Serial.println("ppm");
        
     delay(500);

    //Sentsoreek jasotako balioak IFTTT plataformara bidali
    Serial.print("Connecting to: "); 
    Serial.print(ssid);
    WiFi.begin(ssid, password);  
  
    int timeout = 100 * 4;
    while(WiFi.status() != WL_CONNECTED  && (timeout-- > 0)) {
      delay(250);
      Serial.print(".");
    }
    Serial.println("");
  
    if(WiFi.status() != WL_CONNECTED) {
       Serial.println("Failed to connect, going back to sleep");
    }
  
    Serial.print("WiFi connected in: "); 
    Serial.print(millis());
    Serial.print(", IP address: "); 
    Serial.println(WiFi.localIP());
    Serial.print("Connecting to "); 
    Serial.print(server);
    
    WiFiClient client;
    int retries = 10;
    while(!!!client.connect(server, 80) && (retries-- > 0)) {
      Serial.print(".");
    }
    Serial.println();
    if(!!!client.connected()) {
      Serial.println("Failed to connect...");
    }
    
    Serial.print("Request resource: "); 
    Serial.println(resource);
    
    String jsonObject = String("{\"value1\":\"") + value1 + "\",\"value2\":\"" + value2 + "\",\"value3\":\"" + value3 + "\"}";
  
                        
    client.println(String("POST ") + resource + " HTTP/1.1");
    client.println(String("Host: ") + server); 
    client.println("Connection: close\r\nContent-Type: application/json");
    client.print("Content-Length: ");
    client.println(jsonObject.length());
    client.println();
    client.println(jsonObject);
          
    int timeout2 = 5 * 10;            
    while(!!!client.available() && (timeout2-- > 0)){
      delay(100);
    }
    if(!!!client.available()) {
      Serial.println("No response...");
    }
    while(client.available()){
      Serial.write(client.read());
    }
    
    Serial.println("\nclosing connection");
    client.stop();

    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);    
    Serial.println("Going to sleep now");
    esp_deep_sleep_start();

    }

void loop() {
  
 }
