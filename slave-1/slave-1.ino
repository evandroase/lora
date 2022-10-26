/*
 * Código do Lora que possui os sensores (Slave). 
 * OBS: Os códigos do Slave e GATEWAY são parecidos no começo.
*/

#include <LoRa.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/*Definindo os pinos do LORA e do display*/
#define SCK 5   // GPIO5  SCK
#define MISO 19 // GPIO19 MISO
#define MOSI 27 // GPIO27 MOSI
#define SS 18   // GPIO18 CS
#define RST 14  // GPIO14 RESET
#define DI00 26 // GPIO26 IRQ(Interrupt Request)

/*Pino do Sensor de Turbidez*/
#define SENSOR_TURBIDEZ 13 

/*Frequência de operação do LORA no Brasil*/
#define HIGH_GAIN_LORA     20  /* dBm */
#define BAND               915E6  /* 915MHz de frequencia */

/* Definicoes do OLED */
#define OLED_SDA_PIN    4
#define OLED_SCL_PIN    15
#define SCREEN_WIDTH    128 
#define SCREEN_HEIGHT   64  
#define OLED_ADDR       0x3C 
#define OLED_RESET      16

/* Offset de linhas no display OLED */
#define OLED_LINE1     0
#define OLED_LINE2     10
#define OLED_LINE3     20
#define OLED_LINE4     30
#define OLED_LINE5     40
#define OLED_LINE6     50

/* Definicoes gerais */
#define DEBUG_SERIAL_BAUDRATE    115200
#define RESOLUCAO_ADC            4095 /*Resolução do GPIO*/
#define TENSAO_MAX               5.0  /*Tensão máxima de 5V*/
#define CORRECAO_SINAL           11.0 /*Valor vindo do divisor de tensão*/
#define TOTAL_LEITURAS           100  /*Número de leituras para tirar a média do sinal e atenuar ruído*/ 
 
// Informa o Slave que a que o GATEWAY quer dados
const String GETDATA = "getdata";

// String de retorno para o GATEWAY que contém os dados dos sensores
const String SETDATA = "setdata=";
 
/* Variaveis e objetos globais */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*Função de Setup do Display*/
void setupDisplay(){
    Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
    if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) 
    {
        Serial.println("[LoRa Receiver] Falha ao inicializar comunicacao com OLED");        
    }
    else
    {
        Serial.println("[LoRa Receiver] Comunicacao com OLED inicializada com sucesso");
    
        /* Limpa display e configura tamanho de fonte */
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(WHITE);
    }
}

/*Configurações iniciais do LoRa*/
void setupLoRa()
{ 
  /*Inicializa a comunicação*/
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI00);
 
  /*Inicializa o LoRa*/
  if (!LoRa.begin(BAND))
  {
    /*Se não conseguiu inicializar, mostra uma mensagem no display*/
    display.clearDisplay();
    display.setCursor(0, OLED_LINE3);
    display.println("Reinicie o Lora...");
    display.display();  
    while (1);
  }
 
  /*Ativa o crc*/
  LoRa.enableCrc();
  
  /*Ativa o recebimento de pacotes*/
  LoRa.receive();
}

/*Função que vai ler os dados do sensor (no caso aqui é um contador)*/
String readData()
{
  double Voltagem = 0.0;
  double Turbidez = 0.0;

  /*Tirando a média da voltagem para diminuir o ruído.*/
  for(int i = 0; i < TOTAL_LEITURAS; i++)
  {
    Voltagem += analogRead(SENSOR_TURBIDEZ) * TENSAO_MAX * CORRECAO_SINAL / RESOLUCAO_ADC;
  }  
   
  Voltagem = Voltagem / TOTAL_LEITURAS;
  /*Correção para turbidez anular em água pura*/
  
  if(Voltagem > 4.2)
  {
    Voltagem = 4.2;  
  }
  
  Turbidez = (-1) * (1120.4) * Voltagem * Voltagem + 5742.3 * Voltagem + (-1)*4352.9;
  
  Serial.println(Turbidez);
  return String(Turbidez);
}


/*Setup do slave*/
void setup()
{
    Serial.begin(115200);
    /*Chama a configuração inicial do display*/
    setupDisplay();
    
    /*Chama a configuração inicial do LoRa*/
    setupLoRa();
    display.setCursor(0, OLED_LINE3);
    display.println("Slave esperando...");
    display.display();  
    pinMode(SENSOR_TURBIDEZ, INPUT);
}

/*Programa principal*/
void loop()
{
  /*Tenta ler o pacote*/
  int packetSize = LoRa.parsePacket();
 
  /*Verifica se o pacote possui a quantidade de caracteres que esperamos*/
  if (packetSize == GETDATA.length())
  {
    String received = "";
 
    /*Armazena os dados do pacote em uma string*/
    while(LoRa.available()){
      received += (char) LoRa.read();
    }
  
  if(received.equals(GETDATA)){
      /*Simula a leitura dos dados*/
      String dados = readData();
      Serial.println("Criando pacote para envio");
      
      /*Cria o pacote para envio*/
      LoRa.beginPacket();
      LoRa.print(SETDATA + dados);
      Serial.print("string");
      Serial.println(SETDATA + dados);
      
      /*Finaliza e envia o pacote*/
      LoRa.endPacket();
      
      /*Mostra no display*/
      display.clearDisplay();
      display.setCursor(0, OLED_LINE3);
      display.println("Enviou: ");
      display.println(String(dados));
      display.display();  
    }
  }
}
