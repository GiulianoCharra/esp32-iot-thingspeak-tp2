#include <WiFi.h>

const char* ssid = "TU_SSID";
const char* password = "TU_PASSWORD";

void conectarWifi() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}
