/*
Monitor de Vagas de Estacionamento
[SENSOR LoRa]

Trabalho de Conclusão de Curso
Rafael Krohliing Goldner

IFES Campus Vitória, 2022
Descrição:
Este código corresponde a um programa a ser carregado em uma placa ESP32 com o sensor ultrassônico HC-SR04 e o transceiver RFM95W
para transmissão de dados em uma frequência de 915 MHz.
O sensor enviará a informação ao gateway sempre que for solicitado a fazê-lo e, em seguida, entrará em modo "sleep",
evitando interferência com os outros sensores e reduzindo o consumo de bateria.
*/

//Definição de bibliotecas
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

//Declaração dos dispositivos
Ultrasonic sensor(TRIGGER,ECHO);

String ID = "V1"; //Variável para identificar a vaga do sensor. Esta String é diferente para cada um dos sensores

//Limiar para comparação com a leitura do sensor ultrassônico
float limiar = 60;

//Variável de controle para situação da vaga
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
  LoRa.sleep();
}

//Função para leitura do dado do sensor ultrassônico
float readDistance(){
  float dist = sensor.read(CM); //Lê a distância medida em centímetros
  Serial.print("Distância lida: ");
  Serial.println(dist);
  return dist; //Retorna a distância medida
}

//Função para envio de mensagem por LoRa ao gateway
void sendMessage(String msg){  
  LoRa.beginPacket();  
  LoRa.print(ID);
  LoRa.print("-");
  LoRa.print(msg);
  LoRa.endPacket();
  LoRa.sleep();  //Ativa o modo "sleep", evitando interferência com outros sensores e o gateway
}

void onReceive(int packetSize){
  
  if(packetSize == 0) return; //Se nenhuma mensagem for recebida, não faz nada

  String server_msg = "";

  while(LoRa.available()){
    server_msg += LoRa.readString();
  }

  Serial.println(server_msg);

  if(server_msg == ID+"read"){  //Se a mensagem for para este sensor, realiza a leitura da distância e verifica a situação da vaga
  int dist_atual =  int(readDistance());  
  if(dist_atual < limiar)
  {
    vaga_livre = false; //String correspondente: "0"
  }
  else
  {
    vaga_livre = true; //String correspondente: "1"
  }
   sendMessage(String(vaga_livre)); //Envia mensagem ao gateway
  }
  else
  {
    return;
  }
}

void loop() {
  onReceive(LoRa.parsePacket());  //Recebe os dados enviados pelo gateway e realiza a leitura do sensor
}
