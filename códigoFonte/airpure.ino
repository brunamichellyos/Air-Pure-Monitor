/*Bibliotecas utilizadas*/
#include <PubSubClient.h>
#include <DHT.h>
#include <WiFi.h>
#include <SPI.h>
#include <Adafruit_CCS811.h>

/*Definir os pinos dos sensor*/
#define dhtPin 4 //Sensor de temperatura e umidade.
#define RXD2 16 //Sensor NIRD
#define TXD2 17 //Sensor NIRD
//#define ldrPin 39 //Sensor de luminosidade.

/*Configuração de sensores.*/
//DHT11
#define dhtType DHT11 //Tipo do sensor DHT.
DHT dht(dhtPin, dhtType);
//CSS811
Adafruit_CCS811 ccs;

//Variaveis de leitura.
float temp; //Temperatura em graus celsius.
float umid; //Umidade relativa.
float eco2; //Equivalente de Dióxido de carbono.
float voc; //Total de compostos organicos voláteis.
float tempCCS; //Temperatura aproximada no CCS811.
float valorCO2; //Dióxido de carbono - NIRD. 

/*Configurações de rede e cliente WiFi*/
char ssid[] = "MJ"; //nome da rede.
char pass[] = "12345678"; //senha da rede.
//char ssid[] = "GVT-A358"; //nome da rede.
//char pass[] = "7603000706"; //senha da rede.
char mqttUserName[] = "airpure"; //nome de usuário do MQTT
char mqttPass[] = "GGY85IH2NDA1YUP5"; //chave de acesso do MQTT.
char writeAPIKey[] = "5DZ1DA6O4RMV016W"; //chave de acesso do canal thingspeak.
long channelID = 789990; //Identificação do canal thingspeak.

/*Definir identificação de cliente randomica.*/
static const char alphanum[] = "0123456789""ABCDEFGHIJKLMNOPQRSTUVWXYZ""abcdefghijklmnopqrstuvwxyz";

/*Inicializar a biblioteca do cliente wifi*/
WiFiClient client;

/*Inicializar a biblioteca pubsubclient e definir o broker MQTT thingspeak*/
PubSubClient mqttClient(client);
const char* server = "mqtt.thingspeak.com";

/*Variavel para ter controle da ultima conexão e intervalo de publicação dos dados*/
unsigned long lastConnectionTime = 0;
const unsigned long postingInterval = 20000L; //Postar a cada 20 segundos.

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); //Iniciar porta serial.
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2); //Iniciar porta serial do sensor NIRD.
  int status = WL_IDLE_STATUS; //Setar uma conexão wifi temporaria.

  //Inicializar sensor CCS811 - Sensor de Gás.
  if(!ccs.begin()){
    Serial.println("Falha ao iniciar o CCS811! Checar conexão dos fios.");
    while(1);
  }
  
  //Inicializar sensor DHT11 - Sensor de temperatura e umidade.
  dht.begin();
     
  //Calibrar sensor de temperatura do CCS811
  while(!ccs.available());
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);
  
  //Configurando o modo dos pinos
  pinMode(dhtPin, INPUT);
  //pinMode(ldrPin, INPUT);
  
  /*Tentativa de conexão a rede WiFi*/
  while(status != WL_CONNECTED){
    status = WiFi.begin(ssid, pass); //Conectar a rede WiFi WPA/WPA2.
    delay(5000);
    }
  Serial.println("Conectado ao WiFi: ");
  Serial.println(ssid);
  mqttClient.setServer(server, 1883); //Seta as configurações de acessa ao Broker MQTT.
}

void loop() {
  // put your main code here, to run repeatedly:
  /*Reconectar ao cliente MQTT, caso não esteja conectado.*/
  if(!mqttClient.connected()){
    reconnect();
    }

    mqttClient.loop(); //Chama a função loop continuamente para estabelecer a conexão do servidor

    /*Caso tenha passado o tempo do intervalo, publicar na thingspeak.*/
    if(millis() - lastConnectionTime > postingInterval){
      mqttpublish();
      }
}

/*Função para conectar com o MQTT Broker.*/
void reconnect(){
  char clientID[9]; //identificação do cliente.

  /*Gerar clientID*/
  while(!mqttClient.connected()){
    Serial.print("Tentando conexão MQTT...");
    for(int i=0; i<8; i++){
      clientID[i] = alphanum[random(51)];
      }
  clientID[8] = '\0';

  /*Conectar ao Broker MQTT*/
  if(mqttClient.connect(clientID, mqttUserName, mqttPass)){
    Serial.println("Conectado.");
    }else{
      Serial.print("Failed, rc=");
      /*Verificar o porque ocorreu a falha.*/
      //Ver em: https://pubsubclient.knolleary.net/api.html#state explicação do código da falha.
      Serial.print(mqttClient.state());
      Serial.println("Tentar novamente em 5 segundos.");
      delay(5000);
      }
   }
 }

 /*Função para publicar os dados no canal da thingspeak.*/
 void mqttpublish(){
  /*Leitura dos valores dos sensores.*/
  //DHT11
  temp = dht.readTemperature(); //Ler temperatura do sensor DHT11.
  umid = dht.readHumidity(); //Ler umidade do sensor DHT11.
  //Verifica se a leitura não um número.
  if(isnan(umid) || isnan(temp)){
  Serial.println("Erro de leitura DHT11!");
  return;
  }
  
  //LDR 5mm
  //int ldr = analogRead(ldrPin); //Leitura do ldr - sensor de luminosidade.

  //CCS811
   if(ccs.available()){
    tempCCS = ccs.calculateTemperature(); //Ler temperatura do sensor CCS811.
    if(!ccs.readData()){//Caso não tenha dados sendo lidos, pegar valores lidos.
      eco2 = ccs.geteCO2(); //Ler eCO2 do sensor CCS811.
      voc = ccs.getTVOC(); //Ler TVOC do sensor CCS811.
    }
    else{
      Serial.println("Erro de leitura CCS811!");
      while(1);
    }
  }

  //NIRD
  const byte comando[9] =  {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
  byte response[9];

  for(int i = 0; i<9; i++){
    Serial2.write(comando[i]);
    }

   delay(30);

   if(Serial2.available()){
    for(int i=0; i<9; i++){
      response[i] = Serial2.read();
      }
    int responseHigh = (int)response[2];
    int responseLow = (int)response[3];

    valorCO2 = ((responseHigh<<8)+responseLow);
    }
 
  /*Criar String de dados para enviar a thingspeak.*/
  String dados = String("field1=" + String(temp, 2) + "&field2=" + String(umid, 2) + "&field3=" +String(eco2, 2)+ "&field4=" +String(voc, 2)+ "&field5=" + String(valorCO2));
  int tamanho = dados.length();
  char msgBuffer[tamanho];
  dados.toCharArray(msgBuffer,tamanho+1);
  Serial.println(msgBuffer);

  /*Cria uma String de tópico e publica os dados no canal da thingspeak.*/
  String topicString = "channels/" + String(channelID) + "/publish/"+String(writeAPIKey);
  tamanho = topicString.length();
  char topicBuffer[tamanho];
  topicString.toCharArray(topicBuffer, tamanho+1);

  mqttClient.publish(topicBuffer, msgBuffer);

  lastConnectionTime = millis();
  }
