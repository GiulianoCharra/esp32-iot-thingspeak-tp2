#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
const int LED1 = 23;

//Credenciales red Wifi
const char *ssid = "Redmi11";
const char *password = "esp32000";

//Telegram BOT Token
const String botToken = "7729772647:AAErwcz7tKA1EL3b9bSuGoInb0A1le4jqww";
//const unsigned long tiempo = 1000; //tiempo medio entre mensajes de escaneo
const unsigned long botMTBS = 1000;  //mean time between scan messages
unsigned long botLastScan;
uint8_t ledstatus = 0;

//objects
WiFiClientSecure secured_client;
UniversalTelegramBot bot(botToken, secured_client);

//void setup() {
//  pinMode(LED1,OUTPUT);
//  Serial.begin(115200);
//  Serial.print("Connecting to WiFi");
//  WiFi.begin("Wokwi-GUEST", "", 6);
//  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(100);
//    Serial.print(".");
//  }
//  Serial.println(" Connected!");
//}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  pinMode(LED1, OUTPUT);
  delay(30);
  Serial.print("Conectar a la red Wifi ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
  secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print("·");
    delay(500);
  }
  Serial.print("\nWifi conectado. Direccion IP: ");
  Serial.println(WiFi.localIP());
}

void handleNewMessages(int numNewMessages) {
  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (from_name == "") {
      from_name = "Guest";

      if (text == "/led+") {
        for (int ciclo = 0; ciclo <= 255; ciclo++) {
          analogWrite(LED1, ciclo);
          delay(5);
        }
        bot.sendMessage(chat_id, "Led mas", "");
      }


      if (text == "/led-") {
        for (int ciclo = 255; ciclo >= 0; ciclo--) {
          analogWrite(LED1, ciclo);
          delay(5);
        }
        bot.sendMessage(chat_id, "Led menos", "");
      }

      if (text == "/start") {
        String welcome = "Welcome to Universal Arduino Telegram Bot library, ";  //+ from_name + "
        welcome += "This is flash Led Bot example.\n\n";
        welcome += "/led+ : aumenta brillo led \n";
        welcome += "/led- : disminuye brillo led \n";
        bot.sendMessage(chat_id, welcome, "Markdown");
      }
    }
  }
}

  void loop() {
    if (millis() - botLastScan > botMTBS) {
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      while (numNewMessages) {
        Serial.println("Respuesta obtenida");
        handleNewMessages(numNewMessages);
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
      botLastScan = millis();
    }
  }
