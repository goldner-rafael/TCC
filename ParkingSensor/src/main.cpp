#include <Arduino.h>
#include <Ultrasonic.h> //https://github.com/ErickSimoes/Ultrasonic
#include <SPI.h>
#include <Wire.h> 
#include <LoRa.h> //https://github.com/sandeepmistry/arduino-LoRa

//Pinout para transceptor RFM95W
#define MISO 19
#define MOSI 23
#define SCK 18
#define NSS 5
#define RST 14
#define DIO0 2

//Pinout para sensor HC-SR04
#define TRIGGER 26
#define ECHO 25

//Definição de endereços
byte Gateway = 0xFF; //Endereço do gateway
byte Node1 = 0xBB; //Endereço do sensor (cada sensor tem um endereço diferente)

//Declaração dos dispositivos
Ultrasonic sensor(TRIGGER,ECHO);

String ID = "V1"; //Variável para identificar a vaga do sensor

float limiar = 60;

bool vaga_livre = true;

void setup() {
  //Inicialização do Monitor Serial
  //O Monitor Serial é utilizado para debug do código no VSCode
  Serial.begin(9600);

  //Inicialização do LoRa
  LoRa.setPins(NSS,RST,DIO0);
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  Serial.println("Inicialização do LoRa OK!");
}

//Função para leitura do dado do sensor ultrassônico
float readDistance(){
  float dist = sensor.read(CM);
  Serial.print("Distância lida: ");
  Serial.println(dist);
  return dist;
}

//Função para envio de mensagem por LoRa ao gateway
void sendMessage(String msg){
  LoRa.beginPacket();
  LoRa.print(ID);
  LoRa.print("-");
  LoRa.print(msg);
  LoRa.endPacket();
}

//Função para execução ao receber solicitação do gateway
void onReceive(int packetSize){
  if(packetSize == 0) return; //Se não houver pacote a ser enviado, não faz nada

  String server_msg = "";

  while(LoRa.available()){
    server_msg += LoRa.readString();
  }

   //Envia a leitura até o servidor
  if(server_msg == "read"){
  int dist_atual =  int(readDistance());  
  if(dist_atual < limiar)
  {
    vaga_livre = false;
  }
  else
  {
    vaga_livre = true;
  }
   sendMessage(String(vaga_livre));
  }
  }
}

void loop() {
  onReceive(LoRa.parsePacket());
}
