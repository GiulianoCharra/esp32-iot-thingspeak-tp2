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


#include <ThingSpeak.h>
#include <WiFi.h>
#include <DHT.h>

// Configuración de ThingSpeak
unsigned long channelID = 2745994; // Reemplaza con tu ID de canal
const char* WriteAPIKey = "12KRBEBJB75FKDBN";

// Configuración de pines y objetos
DHT dht(33, DHT22);
WiFiClient cliente;

// Variables globales
int contadorPulsador = 0; // Contador de pulsaciones

void setup() {
    conectarWifi();
    ThingSpeak.begin(cliente);
    dht.begin();
    pinMode(4, INPUT_PULLUP); // Pin del pulsador
    attachInterrupt(digitalPinToInterrupt(4), contarPulsaciones, FALLING);
}

void loop() {
    static unsigned long lastTime = 0;
    const unsigned long intervalo = 20000; // 20 segundos

    if (millis() - lastTime > intervalo) {
        subirDatosThingSpeak();
        lastTime = millis();
    }
}

void contarPulsaciones() {
    static unsigned long ultimoTiempoPulsacion = 0;
    const unsigned long debounce = 200; // 200 ms de antirrebote

    if (millis() - ultimoTiempoPulsacion > debounce) {
        contadorPulsador++;
        ultimoTiempoPulsacion = millis();
    }
}

void subirDatosThingSpeak() {
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    int estadoPote = analogRead(32); // Lectura del potenciómetro

    if (!isnan(temperatura) && !isnan(humedad)) {
        ThingSpeak.setField(1, temperatura);
        ThingSpeak.setField(2, humedad);
        ThingSpeak.setField(3, estadoPote);
        ThingSpeak.setStatus("Pulsador presionado " + String(contadorPulsador) + " veces");

        int httpCode = ThingSpeak.writeFields(channelID, WriteAPIKey);
        if (httpCode == 200) {
            Serial.println("Datos subidos correctamente a ThingSpeak.");
        } else {
            Serial.println("Error al subir los datos: " + String(httpCode));
        }
    } else {
        Serial.println("Error en la lectura del sensor DHT22");
    }
}


WiFiServer server(80);

void setup() {
    conectarWifi();
    server.begin();
    Serial.println("Servidor web iniciado.");
}

void loop() {
    WiFiClient client = server.available();
    if (client) {
        String currentLine = "";
        while (client.connected()) {
            if (client.available()) {
                char c = client.read();
                if (c == '\n') {
                    if (currentLine.length() == 0) {
                        client.println("HTTP/1.1 200 OK");
                        client.println("Content-type:text/html");
                        client.println("Connection: close");
                        client.println();
                        
                        // Leer datos de ThingSpeak
                        String datos = obtenerDatosThingSpeak();
                        client.println(datos);
                        
                        client.println();
                        break;
                    } else {
                        currentLine = "";
                    }
                } else if (c != '\r') {
                    currentLine += c;
                }
            }
        }
        client.stop();
        Serial.println("Cliente desconectado.");
    }
}

String obtenerDatosThingSpeak() {
    // Reemplaza con tu código para obtener datos de ThingSpeak usando un cliente HTTP
    return "<html><body><h1>Datos de ThingSpeak</h1></body></html>";
}
