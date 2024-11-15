#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Adafruit_SH110X.h>

// Credenciales de WiFi
const char* ssid = "Tu_SSID";
const char* password = "Tu_PASSWORD";

// Configuración de ThingSpeak
unsigned long channelID = 2745994; // Reemplaza con tu ID de canal
const char* WriteAPIKey = "12KRBEBJB75FKDBN";
const String readAPIKey = "TU_READ_API_KEY";  // Reemplaza con tu Read API Key
const String server = "http://api.thingspeak.com";

// Pines y objetos
DHT dht(33, DHT22);
WiFiClient cliente;
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
WiFiServer serverWeb(80);

// Variables globales
int contadorPulsador = 0;
unsigned long lastTime = 0;
const unsigned long intervalo = 20000; // 20 segundos

void conectarWifi() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
}

void setup() {
    conectarWifi();
    ThingSpeak.begin(cliente);
    dht.begin();
    pinMode(4, INPUT_PULLUP); // Pin del pulsador
    attachInterrupt(digitalPinToInterrupt(4), contarPulsaciones, FALLING);

    // Configuración de la pantalla
    display.begin(0x3c, true);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Iniciar servidor web
    serverWeb.begin();
    Serial.println("Servidor web iniciado.");
}

void loop() {
    if (millis() - lastTime > intervalo) {
        subirDatosThingSpeak();
        lastTime = millis();
    }
    manejarClienteWeb();
}

void contarPulsaciones() {
    static unsigned long ultimoTiempoPulsacion = 0;
    const unsigned long debounce = 200;

    if (millis() - ultimoTiempoPulsacion > debounce) {
        contadorPulsador++;
        ultimoTiempoPulsacion = millis();
    }
}

void subirDatosThingSpeak() {
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    int estadoPote = analogRead(32);

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

void manejarClienteWeb() {
    WiFiClient client = serverWeb.available();
    if (client) {
        Serial.println("Cliente conectado");
        String header;
        bool requestHandled = false;

        while (client.connected() && !requestHandled) {
            if (client.available()) {
                char c = client.read();
                header += c;
                if (c == '\n' && header.endsWith("\r\n\r\n")) {
                    requestHandled = true;
                    mostrarPaginaWeb(client);
                }
            }
        }
        client.stop();
        Serial.println("Cliente desconectado");
    }
}

void mostrarPaginaWeb(WiFiClient& client) {
  String datos = obtenerDatosThingSpeak();

  // Generar la respuesta HTML con CSS embebido
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  client.println("<!DOCTYPE html>");
  client.println("<html>");
  client.println("<head>");
  client.println("<title>Datos de ThingSpeak</title>");
  client.println("<style>");
  client.println("body { font-family: Arial, sans-serif; background-color: #f4f4f4; margin: 0; padding: 20px; }");
  client.println("h1 { color: #333; }");
  client.println("pre { background-color: #fff; padding: 10px; border: 1px solid #ddd; }");
  client.println(".container { max-width: 800px; margin: auto; background: #fff; padding: 20px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }");
  client.println("</style>");
  client.println("</head>");
  client.println("<body>");
  client.println("<div class='container'>");
  client.println("<h1>Datos obtenidos de ThingSpeak</h1>");
  client.println("<pre>" + datos + "</pre>");
  client.println("</div>");
  client.println("</body>");
  client.println("</html>");

  client.println();
}

String obtenerDatosThingSpeak() {
    HTTPClient http;
    String url = server + "/channels/" + String(channelID) + "/feeds.json?api_key=" + readAPIKey + "&results=1";
    http.begin(url);

    int httpResponseCode = http.GET();
    String payload;

    if (httpResponseCode > 0) {
        payload = http.getString();
        Serial.println("Datos obtenidos correctamente");
    } else {
        payload = "Error al obtener datos: " + String(httpResponseCode);
        Serial.println(payload);
    }

    http.end();
    return payload;
}
