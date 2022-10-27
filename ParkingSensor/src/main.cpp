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

String mensagem = ""; //Variável para armazenar a mensagem a ser enviada

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
void sendMessage(String msg, byte Receiver, byte Sender){
  LoRa.beginPacket();
  LoRa.write(Receiver);
  LoRa.write(Sender);
  LoRa.print(msg);
  LoRa.endPacket();
}

//Função para execução ao receber solicitação do gateway
void onReceive(int packetSize){
  if(packetSize == 0) return; //Se não houver pacote a receber, não faz nada

  //Leitura do cabeçalho
  int rec = LoRa.read();
  byte send = LoRa.read();
  byte inc_msg_id = LoRa.read();
  byte inc_msg_len = LoRa.read();

  //Variável para armazenamento da mensagem recebida
  String inc_msg = "";

  //Recebe a mensagem enviada pelo gateway
  while(LoRa.available()){
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
  Serial.println(inc_msg);

  // Executa a função abaixo caso a mensagem enviada esteja solicitando a situação da vaga 
  if(inc_msg == "read_status"){
    String message = String(readDistance()); //Lê a distância medida pelo sensor e converte o valor para string
    sendMessage(message,Gateway,Node1); //Envia a mensagem lida
  }
}

void loop() {
  onReceive(LoRa.parsePacket());
}