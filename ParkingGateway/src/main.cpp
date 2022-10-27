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

//Pinout para declaração dos leds Rx e Tx
const int led_send = 26;
const int led_rec = 25;

//Declaração dos endereços
byte Gateway = 0xFF;

//Inclusão dos endereços dos Nodes de sensor de estacionamento
//ATENÇÃO: não repetir o mesmo endereço para dois Nodes diferentes
byte Node1 = 0xBB;

//Contador para loop do programa principal
int nodeCount = 0;

//Variável para armazenar string a ser exibida no display
String SenderNode = "";

//Definição de SSID e PASSWORD para autenticação na rede Wi-fi
//ATENÇÃO: Substituir as strings abaixo pelos valores correspondentes à rede Wi-Fi ao qual o gateway irá se conectar
#define SSID "SSID"
#define PASSWORD "PASSWORD"

//Declaração dos dispositivos
LiquidCrystal_I2C lcd(0x27,20,4); //Display de LCD 20x4

int status_wifi = WL_IDLE_STATUS;

unsigned long previousMillis=0;
unsigned long currentMillis=0;

unsigned long interval = 1; //

//Função para inicialização da rede Wi-Fi
void startWiFi(){
  lcd.clear();
  Serial.println("Conectando ao Wi-Fi...");  
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Wi-fi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
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

void setup() {
  //Inicialização do Monitor Serial
  //O Monitor Serial é utilizado para debug do código no VSCode
  Serial.begin(9600);

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

  startWiFi();
}

//Função para enviar mensagem do gateway para os sensores solicitando a leitura dos dados
void sendMessage(String msg, byte MasterNode, byte Receiver){
  LoRa.beginPacket();
  LoRa.write(MasterNode);
  LoRa.write(Receiver);
  LoRa.print(msg);
  LoRa.endPacket();
}

void onReceive(int packetSize){
  if(packetSize == 0) return; //Se não for recebido nenhum pacote, não faz nada
  int rec = LoRa.read();
  byte sender = LoRa.read();
  byte inc_msg_id = LoRa.read();
  byte inc_msg_len = LoRa.read();
  switch(sender){ //Estrutura para verificar de qual sensor se quer fazer a leitura
    case 0xBB:
    SenderNode = "Vg1: ";
    break;
    //Resto da estrutura Switch com endereços a definir para múltiplos nodes. Inseridos como comentário, por enquanto.
    /*
    case 0xC0:
    SenderNode = "Vg2: ";
    break;
    case 0xC3:
    SenderNode = "Vg3: ";
    break;
    case 0xC9:
    SenderNode = "Vg4: ";
    break;
    case 0xCC:
    SenderNode = "Vg5: ";
    break;
    case 0xCF:
    SenderNode = "Vg6: ";
    break;
    */
    default:
    break;
  }

  //Obtém mensagem enviada pelo sensor
  String inc_msg = "";
  while (LoRa.available()){
    inc_msg += (char)LoRa.read();
  }

  //Verifica se o comprimento da mensagem está correto
  if(inc_msg_len != inc_msg.length()){
    Serial.println("Erro: comprimento da mensagem não é válido");
    return; //Aborta a execução da função em caso de erro
  }

  //Verifica se a mensagem enviada é para este dispositivo
  if(rec != Node1 && rec != Gateway){
    Serial.println("Erro: mensagem não é para este dispositivo");
    return; //Aborta a execução da função caso a mensagem não seja para este dispositivo
  }

  switch(sender){ //Estrutura para display dos dados da situação das vagas de estacionamento. Parte em implementação.
    case 0xBB:
    lcd.setCursor(0,1);
    lcd.print(sender);
    lcd.setCursor(0,4);
    lcd.print(inc_msg);
    break;

    default:
    break;
  }
}

void loop() {
  //Verifica se o Wi-fi está ativo
  if(status_wifi != WL_CONNECTED){
    reconnectWiFi();
  }

  currentMillis = millis();
  if(currentMillis-previousMillis >= interval){
    //Imprime no display a primeira linha
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sit. Vagas: ");

    String message = "read"; //Envia comando para ler os sensores
    byte nodeToSend; //Variável para guardar o endereço do sensor a ser lido  

    //As estruturas "if" e "switch" abaixo são responsáveis pelo loop de leitura dos sensores de estacionamento
    if(nodeCount > 5){
      nodeCount = 0;
    }

    switch(nodeCount){
      case 0:
      nodeToSend = 0xBB;
      break;
      /*
      case 1:
      nodeToSend = 0xC0;
      break;
      case 2:
      nodeToSend = 0xC3;
      break;
      case 3:
      nodeToSend = 0xC9;
      break;
      case 4:
      nodeToSend = 0xCC;
      break;
      case 5:
      nodeToSend = 0xCF;
      break;
      */
    }
    nodeCount++;
    sendMessage(message,Gateway,nodeToSend); //Envia a mensagem ao sensor correspondente
    onReceive(LoRa.parsePacket()); //Chama a função onReceive após receber os dados
  }
  previousMillis = currentMillis;
}