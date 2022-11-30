/*
Monitor de Vagas de Estacionamento
[GATEWAY LoRa]

Trabalho de Conclusão de Curso
Rafael Krohliing Goldner

IFES Campus Vitória, 2022

Descrição:
Este código corresponde a um programa a ser carregado em um gateway LoRa simplificado, utilizando o transceiver RFM95W
para transmissão de dados em uma frequência de 915 MHz.

O GATEWAY LoRa é responsável por interligar o servidor de aplicação e os sensores de estacionamento, através do uso da
tecnologia LoRa e do Wi-Fi.

Por se tratar de um sistema com apenas um gateway, não foi necessário utilizar a tecnologia LoRaWAN. Contudo, a LoRaWAN,
para o efetivo uso, necessitaria a utilização de um servidor específico para esse fim, como o The Things Network (TTN),
que NÃO suporta gateways LoRa/LoRaWAN baseados em transceivers, como o deste projeto.

Dito isso, as definições de comunicação com os dispositivos precisou ser feita diretamente dentro deste código.
*/


//Declaração de bibliotecas e pacotes utilizados
#include <Arduino.h>
#include <LiquidCrystal_I2C.h> //https://github.com/fdebrabander/Arduino-LiquidCrystal-I2C-library
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <LoRa.h> //https://github.com/sandeepmistry/arduino-LoRa
#include <ArduinoJson.h> //https://www.arduino.cc/reference/en/libraries/arduinojson/
#include <PubSubClient.h> //https://www.arduino.cc/reference/en/libraries/pubsubclient/
#include <ThingsBoard.h> //https://github.com/thingsboard/thingsboard-arduino-sdk

//Pinout para transceptor RFM95W
#define MISO 19
#define MOSI 23
#define SCK 18
#define NSS 5
#define RST 14
#define DIO0 2

//Definição do servidor Thingsboard local
#define TB_SERVER "192.168.0.103"
/*
O servidor local do Thingsboard deve ser montado em um computador com Linux
(podendo ser um Raspberry Pi), e o endereço IP do servidor pode ser obtido
através do comando

hostname -I
*/

//Definição do token de acesso do ESP32 para a plataforma de IoT
#define TOKEN "2iepfZhttoJXkhHNSHrf" //Token gerado automaticamente dentro da plataforma

//Variáveis para acesso do dispositivo à plataforma de IoT Thingsboard (servidor local)
bool subscribed = false;
WiFiClient espClient;
ThingsBoard tb(espClient);

//Pinout para declaração dos leds Rx e Tx (que sinalizam transmissão e recepção de dados, respectivamente)
const int led_send = 26;
const int led_rec = 25;

//Contador de tempo
int timecount = 0;

//Contador para loop do programa principal
int nodeCount = 0;

//Definição de SSID e PASSWORD para autenticação na rede Wi-fi
//ATENÇÃO: Substituir as strings abaixo pelos valores correspondentes à rede Wi-Fi ao qual o gateway irá se conectar
//Se o dispositivo for conectado a uma rede wi-fi diferente, o SSID e o PASSWORD precisarão de ser trocados
#define SSID "SSID"
#define PASSWORD "PASSWORD"

//Declaração do display LCD 20x4
LiquidCrystal_I2C lcd(0x27,20,4); //O display utiliza a interface I2C com o endereço 0x27

//Variável para armazenar status da rede Wi-fi
int status_wifi = WL_IDLE_STATUS;

//Variáveis de controle para executar loop do programa principal sem o uso da função "delay"
unsigned long previousMillis=0;
unsigned long currentMillis=0;

unsigned long interval = 2000; //Intervalo de tempo entre a execução de cada loop (leitura de UM dos sensores)

//Função para inicialização da rede Wi-Fi
void startWiFi(){
  lcd.clear();
  Serial.println("Conectando ao Wi-Fi...");  
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { //Executa esta rotina enquanto a rede wi-fi não inicializa
    delay(500);
    Serial.print(".");
  }
  //Exibe informações no monitor serial do Arduino/VSCode
  Serial.println("Wi-fi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  //Exibe informações no display de LCD
  lcd.setCursor(0,0);
  lcd.print("Wi-fi conectado");
  lcd.setCursor(0,1);
  lcd.print("IP: ");
  lcd.setCursor(4,1);
  lcd.print(WiFi.localIP());
}

//Função para reconexão à rede Wi-fi em caso de queda
void reconnectWiFi(){
  status_wifi = WiFi.status();
  if (status_wifi != WL_CONNECTED) {
    WiFi.begin(SSID,PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Wi-fi reconectado");
  }
}

//Função para enviar mensagem do gateway para os sensores, com a tecnologia LoRa, solicitando a leitura dos dados
void sendMessage(String msg){  
  digitalWrite(led_send,HIGH);
  LoRa.beginPacket();  
  LoRa.print(msg);
  LoRa.endPacket();
  digitalWrite(led_send,LOW);  
}

//Função para exibir o status das vagas de estacionamento
void displayStatus(String id,String status,int c, int l){
  if(status != "N/D"){ //Esta função só é executada se a string de entrada for diferente de N/D, que indica "Não disponível"
  lcd.setCursor(c,l);
  lcd.print(id);
  lcd.setCursor(c+3,l);
if(status == "0")
{
  lcd.print("Ocup."); //0 para vaga ocupada
}
else if(status == "1")
{
  lcd.print("Livre"); //1 para vaga livre
}
  }
}

//Função executada pelo gateway ao receber um pacote de dados do sensor através do LoRa
void onReceive(int packetSize){
    
  if(packetSize == 0){    
    return; //Se não for recebido nenhum pacote, não faz nada 
  }
  else
  {
    digitalWrite(led_rec,HIGH);
    //Lê a string de identificação do sensor que enviou a informação e avisa no monitor serial
    String sender = LoRa.readStringUntil('-');
    Serial.print(sender);
    Serial.println(" enviou uma informação");   

  //Obtém mensagem enviada pelo sensor
  String status_vaga = "";
  while (LoRa.available()){
    status_vaga += LoRa.readString();
  }
  Serial.println(status_vaga);
  if(status_vaga.length() == 0)
  {
    status_vaga = "N/D"; //Se não for recebida mensagem sobre o status da vaga, coloca o status como "Não Disponível"
  }

  digitalWrite(led_rec,LOW);
  
    //Identifica de qual sensor veio a informação recebida
    if(sender == "V1")
    {    
    displayStatus(sender,status_vaga,0,1); //Exibe o status da vaga no display de LCD
    tb.sendTelemetryInt("S1",status_vaga.toInt());  //Envia os dados pelo Wi-fi à plataforma Thingsboard   
    }    
    if(sender == "V2")
    {    
    displayStatus(sender,status_vaga,0,2);
    tb.sendTelemetryInt("S2",status_vaga.toInt()); 
    }
    if(sender == "V3")
    {      
    displayStatus(sender,status_vaga,0,3);
    tb.sendTelemetryInt("S3",status_vaga.toInt()); 
    }
    if(sender == "V4")
    {      
    displayStatus(sender,status_vaga,10,1);
    tb.sendTelemetryInt("S4",status_vaga.toInt());  
    }
    if(sender == "V5")
    {      
    displayStatus(sender,status_vaga,10,2);
    tb.sendTelemetryInt("S5",status_vaga.toInt());  
    }       
  }
  digitalWrite(led_rec,LOW);
}

//Função para inicialização do programa
void setup() {
  //Inicialização do Monitor Serial
  //O Monitor Serial é utilizado para debug do código no VSCode
  Serial.begin(9600);

   //Inicializa LEDs de sinalização
  pinMode(led_rec,OUTPUT);
  digitalWrite(led_rec,LOW);
  pinMode(led_send,OUTPUT);
  digitalWrite(led_send,LOW);

  //Inicializa o display de LCD
  lcd.init();
  lcd.backlight();

  //Inicialização do LoRa
  LoRa.setPins(NSS,RST,DIO0);
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  Serial.println("Inicialização do LoRa OK!");
  lcd.setCursor(0,0);
  lcd.print("Inic. LoRa");
  lcd.setCursor(0,1);
  lcd.print("Concluida");
  //Inicializa o Wi-fi
  startWiFi();
  lcd.clear();  
}

//PROGRAMA PRINCIPAL
void loop() {  
  //Verifica se o Wi-fi está ativo  
  
  if(status_wifi != WL_CONNECTED){
    reconnectWiFi();
  }

   //Tenta a conexão com a plataforma ThingsBoard
  if(!tb.connected()){
    subscribed = false;

    Serial.print("Conectando ao servidor ");
    Serial.print(TB_SERVER);
    Serial.print(" com o token ");
    Serial.println(TOKEN);
    if (!tb.connect(TB_SERVER, TOKEN)) {
      Serial.println("Erro de conexão");
      return;
    }
  //Caso já esteja conectado, esta função é pulada
  }

  currentMillis = millis();
  //
  if(currentMillis-previousMillis >= interval){ //Executa este loop uma vez a cada intervalo de tempo
    
    lcd.setCursor(0,0); 
    lcd.print("Sit. Vagas: ");
  
    //Envia a mensagem por LoRa até um sensor definido
    String message = "V"+String(nodeCount+1)+"read";
    sendMessage(message);
    //Informa ao monitor serial que a mensagem foi enviada
    Serial.print("Enviada mensagem ao sensor da vaga ");
    Serial.println(nodeCount+1);

    nodeCount++;
    timecount+=2;
    tb.sendTelemetryInt("cont",timecount); //Atualiza o contador de tempo

    if(nodeCount > 4){
      nodeCount = 0;
    }  
     previousMillis = currentMillis;         
  }             
  onReceive(LoRa.parsePacket()); //Chama a função onReceive após receber algum dado através do LoRa
  tb.loop(); //Mantém o envio de dados ao ThingsBoard     
}
