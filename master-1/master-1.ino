/*
 * Código do GATEWAY que pega os dados dos sensores e envia
 * através do protocolo MQTT para o broker HIVEMQ.
 * 
*/

#include <stdio.h>         
#include <WiFi.h>         
#include <PubSubClient.h>  
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/* 
* Variáveis globais do Wi-Fi e MQTT
*/

/*  Nome da rede wi-fi e senha que o módulo vai se conectar */
const char* ssid_wifi = "nome da rede";  
const char* password_wifi = "senha";   

/* Tópico MQTT para o qual o ESP32 enviará os dados */
const char* MQTT_PUB_TOPIC = "channels/1686838/publish/fields/field1";

/*Credenciais do ThingSpeak*/
const char mqttUserName[] = "JjoBCDMyOBwwDy8DEC8THTY"; 
const char clientID[] = "JjoBCDMyOBwwDy8DEC8THTY";
const char mqttPass[] = "zfZ7rNGc6245Arxyv8zlquxi";


/* Objeto para conexão a Internet através de Wi-Fi */
WiFiClient espClient;     

/* 
  Endereço do broker MQTT HIVEMQ
*/
const char* broker_mqtt = "mqtt3.thingspeak.com"; 

/* 
  Porta do broker MQTT que deve ser utilizada (padrão: 1883) 
  A porta 1883 é usada para casos de mensagens sem criptografia e tem 
  propósitos de prototipação e estudos.
  Em um sistema real é utilizada a porta 8883 com TSL.  
*/
int broker_port = 1883;    

/* Objeto para conexão com broker MQTT */
PubSubClient MQTT(espClient);

/*Definindo os pinos do LORA e do display*/
#define SCK 5   // GPIO5  SCK
#define MISO 19 // GPIO19 MISO
#define MOSI 27 // GPIO27 MOSI
#define SS 18   // GPIO18 CS
#define RST 14  // GPIO14 RESET
#define DI00 26 // GPIO26 IRQ(Interrupt Request)


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

/* Informa o Slave que o GATEWAY quer dados*/
const String GETDATA = "getdata";

/* String de retorno para o GATEWAY que contém os dados dos sensores*/
const String SETDATA = "setdata=";
 
/* Variaveis e objetos globais */
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

/*Função de Setup do Display*/
void setupDisplay()
{
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
        display.setCursor(0, OLED_LINE1);
        display.print("Display carregado!");
        display.display();

    }
}

/*Configurações iniciais do LoRa*/
void setupLoRa()
{ 
  /*Inicializa a comunicação*/
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DI00);
 
  /*Inicializa o LoRa*/
  while (!LoRa.begin(BAND))
  {
    /*Se não conseguiu inicializar, mostra uma mensagem no display*/
    display.setCursor(0, OLED_LINE2);
    display.println("Erro ao inicializar o LoRa!");
    display.display();
  }

  display.setCursor(0, OLED_LINE2);
  display.println("LoRa iniciado!");
  display.display();  
  
  /*Ativa o crc*/
  LoRa.enableCrc();
  
  /*Ativa o recebimento de pacotes*/
  LoRa.receive();
}

/*Intervalo entre os envios*/
#define INTERVAL 5000
 
/*
 * Tempo do último envio. 
  OBS: Tem que ser long porque o Lora pode ficar ligado por muito tempo.
*/
long lastSendTime = 0;


/*Função que manda a requisição dos dados.*/
void send() 
{
  LoRa.beginPacket();
  LoRa.print(GETDATA);
  LoRa.endPacket();
}
      
/* Função: inicaliza conexão wi-fi
 * Parâmetros: nenhum
 * Retorno: nenhum 
 */
void init_wifi(void) 
{  
    connect_wifi();
}

/* Função: Configura endereço do broker e porta para conexão com broker MQTT
 * Parâmetros: nenhum
 * Retorno: nenhum 
 */
void init_MQTT(void)
{
    MQTT.setServer(broker_mqtt, broker_port);
}

/* Função: conecta-se ao broker MQTT (se não há conexão já ativa)
 * Parâmetros: nenhum
 * Retorno: nenhum 
 */
void connect_MQTT(void) 
{
    while (!MQTT.connected()) 
    {
        Serial.print("* Tentando se conectar ao seguinte broker MQTT: ");
        Serial.println(broker_mqtt);
        
        if (MQTT.connect(clientID, mqttUserName, mqttPass, "", 0, false,"" ,true)) 
        {
            Serial.println("Conexao ao broker MQTT feita com sucesso!");
                
            display.setCursor(0, OLED_LINE4);
            display.print("Conectado ao broker!");
            display.display();
        } 
        else 
        {
            display.setCursor(0, OLED_LINE4);
            display.print("Falha ao se conectar ao broker MQTT!");
            display.display();
        }
    }
}

/* Função: connect to wifi network
 * Parâmetros: nenhum
 * Retorno: nenhum 
 */
void connect_wifi(void) 
{
    /* Conecta ao Wi-Fi se não existe conexão e não faz nada caso 
       não exista conexão.
     */
    if (WiFi.status() == WL_CONNECTED)
    {
        return;
    }    
    
    WiFi.begin(ssid_wifi, password_wifi);
    
    while (WiFi.status() != WL_CONNECTED) 
    {

    }

    display.setCursor(0, OLED_LINE3);
    display.print("Conectado a:");
    display.print(ssid_wifi);
    display.display();
}

/* Função: verifica se há conexão wi-fi ativa (e conecta-se caso não haja)
 * Parâmetros: nenhum
 * Retorno: nenhum 
 */
void verify_wifi_connection(void)
{
    connect_wifi(); 
}

void setup(){
  /* Configuração da serial (usada para debug no Serial Monitor */  
  Serial.begin(DEBUG_SERIAL_BAUDRATE);

  /*Configura display*/
  setupDisplay();
  
  /*Configura o LoRa*/
  setupLoRa();


  /* Inicializa conexão wi-fi */
  init_wifi();

  /* inicializa conexão MQTT */
  init_MQTT();
  connect_MQTT();

  delay(1000);
  display.clearDisplay();
  display.setCursor(0, OLED_LINE1);
  display.print("-------GATEWAY-------");
  display.display();
}

void send_mqtt(char* payload)
{
    /* Verifica se a conexão wi-fi está ativa e, em caso negativo, reconecta-se ao roteador */
    verify_wifi_connection();
    /* Verifica se a conexão ao broker MQTT está ativa e, em caso negativo, reconecta-se ao broker */
    connect_MQTT();

    /* Envia a frase "Comunicacao MQTT via ESP32" via MQTT */
    MQTT.publish(MQTT_PUB_TOPIC, payload);  
    display.setCursor(0, OLED_LINE5);
    display.print("Dados: ");
    display.print(payload);
    display.display();
}

void loop(){

  /*
    O GATEWAY pede para o Slave mandar os dados somente se 
    passar um tempo maior que o intervalo determinado (5 minutos)
    OBS: A função millis() pega o tempo desde que o Lora ligou, então
    o lastSendTime sempre recebe + 5 minutos a cada requisição de dados.
  */
  if (millis() - lastSendTime > INTERVAL)
  {
    /*Marcamos o tempo que ocorreu o último envio*/
    lastSendTime = millis();
    /*Envia o pacote para informar ao Slave que o GATEWAY quer receber dados*/
    send();  
  }

    /*Tentativa de leitura do pacote*/
  int packetSize = LoRa.parsePacket();
   
  /*Verifica se o pacote tem o tamanho mínimo de caracteres esperado*/
  if (packetSize > SETDATA.length())
  {
  
    String received = "";
  
    /*Armazena os dados do pacote em uma string*/
    while(LoRa.available())
    {
      received += (char) LoRa.read();
    }
    
    /*Verifica se a string possui o que está contido em "SETDATA"*/
    int index = received.indexOf(SETDATA);
    
    if(index >= 0)
    {
      /*Recupera string após "SETDATA"*/
      String temp = received.substring(SETDATA.length());
    
      /*
        Tempo para o GATEWAY criar o pacote, enviá-lo,
        o Slave receber, ler, criar um novo pacote, enviá-lo
        e o Master receber e ler
      */
      String waiting = String(millis() - lastSendTime);
      
      /*Mostra no display os dados e o tempo que a operação demorou*/
      display.clearDisplay();
      display.setCursor(0, OLED_LINE1);
      display.print("-------GATEWAY-------");      
      display.setCursor(0, OLED_LINE2);
      display.print("Recebeu: " );
      display.print(temp);
      display.setCursor(0, OLED_LINE3);
      display.print("Tempo: ");
      display.print(waiting);
      display.print("ms");
      display.display();

      int str_len = 10; 
      char payload[str_len];
      temp.toCharArray(payload, str_len);

      /*Envia dados para o broker caso tenha recebido mensagem do slave.*/
      if(sizeof(payload) > 0)
      {
        display.setCursor(0, OLED_LINE6);
        display.print(sizeof(payload));
        send_mqtt(payload);  
      }
    }
  }
}
