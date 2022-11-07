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

//Contador para loop do programa principal
int nodeCount = 0;

//Definição do servidor local ThingsBoard
#define TOKEN "2iepfZhttoJXkhHNSHrf"
#define TB_SERVER "192.168.0.102"

//Definição de SSID e PASSWORD para autenticação na rede Wi-fi
//ATENÇÃO: Substituir as strings abaixo pelos valores correspondentes à rede Wi-Fi ao qual o gateway irá se conectar
#define SSID "SSID"
#define PASSWORD "PASSWORD"

//Declaração dos dispositivos
LiquidCrystal_I2C lcd(0x27,20,4); //Display de LCD 20x4

int status_wifi = WL_IDLE_STATUS;

//Variáveis de controle para função millis
unsigned long previousMillis=0;
unsigned long currentMillis=0;

unsigned long interval = 1000;

bool subscribed = false;

WiFiClient espClient;
ThingsBoard tb(espClient);

bool subscribed = false;

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
  pinMode(led_send,OUTPUT);
  pinMode(led_rec,OUTPUT);
  //Inicializa o Wi-Fi após inicializar o LoRa
  startWiFi();
}

//Função para enviar mensagem do gateway para os sensores solicitando a leitura dos dados
void sendMessage(String msg){
  digitalWrite(led_send,HIGH);
  LoRa.beginPacket();  
  LoRa.print(msg);
  LoRa.endPacket();
  digitalWrite(led_send,LOW);
}

void onReceive(int packetSize){
 digitalWrite(led_rec,HIGH);
  if(packetSize == 0){    
    return;
  }
  else
  {
    String sender = LoRa.readStringUntil('-');
    Serial.print(sender);
    Serial.println(" enviou uma informação");
 /* switch(sender){ //Estrutura para verificar de qual sensor se quer fazer a leitura
    case 0xBB:
    SenderNode = "Vg1: ";
    break;        
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
   String status_vaga = "";
   while (LoRa.available()){
    status_vaga += LoRa.readString();
  }
  Serial.println(status_vaga);
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
  //Conecta ao servidor local ThingsBoard (caso não esteja conectado ainda)
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

  }
  //Loop principal do programa
  currentMillis = millis();
  if(currentMillis-previousMillis >= interval){
    previousMillis = currentMillis;
    //Imprime no display a primeira linha
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sit. Vagas: ");

    lcd.clear();
    lcd.setCursor(0,0); 
    lcd.print("Sit. Vagas: ");

    for(int i=0;i<6;i++)
    {
      String message = "V"+String(i+1);
      message += "read";
      sendMessage(message); //Envia a mensagem ao sensor correspondente 
    }   
    
  }
  onReceive(LoRa.parsePacket()); //Chama a função onReceive após receber algum dado  
  }
  tb.loop();
}
