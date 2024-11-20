#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Adafruit_SH110X.h>
#include <ArduinoJson.h>

// Credenciales de WiFi
const char* ssid = "PruebaESP";  // Nombre de la red WiFi
const char* password = "asd12345";    // Contraseña de la red WiFi

// Configuración de ThingSpeak
unsigned long channelID = 2745994; // ID del canal en ThingSpeak
const char* WriteAPIKey = "12KRBEBJB75FKDBN";  // Clave de escritura para el canal
const String readAPIKey = "15RFMQU0NKOMYF21";  // Clave de lectura del canal
const String server = "http://api.thingspeak.com";  // URL base de ThingSpeak

// Pines configurables
const int dhtPin = 33;            // Pin del sensor DHT
const int potePin = 32;           // Pin del potenciómetro
const int botonPin = 19;           // Pin del botón pulsador
const int oledResetPin = -1;      // Pin de reset de OLED (no conectado)
const int botonInterruptPin = 4; // Pin para interrupción del botón

// Objetos de hardware
DHT dht(dhtPin, DHT22);  // Objeto DHT para medir temperatura y humedad
WiFiClient cliente;      // Cliente WiFi para enviar y recibir datos
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, oledResetPin);  // Objeto para la pantalla OLED
WiFiServer serverWeb(80);  // Servidor web en el puerto 80

// Variables globales
int contadorPulsador = 0;  // Contador de pulsaciones del botón
unsigned long lastTime = 0;         // Marca de tiempo para la última subida de datos
const unsigned long intervalo = 20000;  // Intervalo de tiempo para subir datos (20 segundos)


// Función para conectar la ESP32 a la red WiFi
void conectarWifi() {
  if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      Serial.print("Conectando a WiFi");
      while (WiFi.status() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
      }
      Serial.println("\nWiFi conectado");

      // Mostrar la IP correctamente
      IPAddress ip = WiFi.localIP();
      if (ip) { // Validar si la IP es válida
          Serial.print("IP del servidor web: ");
          Serial.println(ip);
          Serial.print("URL de la página: http://");
          Serial.print(ip);
          Serial.println("/");
      } else {
          Serial.println("No se pudo obtener la IP.");
      }
  }
}

// Configuración inicial de la ESP32
void setup() {
    conectarWifi();
    Serial.begin(115200);

    ThingSpeak.begin(cliente);
    dht.begin();
    pinMode(botonPin, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(botonPin), contarPulsaciones, FALLING);


    // Configuración de la pantalla OLED
    display.begin(0x3c, true);
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);

    // Iniciar el servidor web
    serverWeb.begin();
    Serial.println("Servidor web iniciado.");
}

// Bucle principal
void loop() {
    conectarWifi(); // Verificar conexión WiFi
    if (millis() - lastTime > intervalo) {
        subirDatosThingSpeak();
        lastTime = millis();
    }
    manejarClienteWeb();
}

// Función de interrupción para contar las pulsaciones del botón
void contarPulsaciones() {
    static unsigned long ultimoTiempoPulsacion = 0;
    const unsigned long debounce = 200;
    if (digitalRead(botonPin) == LOW && (millis() - ultimoTiempoPulsacion) > debounce) {
        contadorPulsador++;
        ultimoTiempoPulsacion = millis();
    }
}

// Función para contar las pulsaciones del botón
// void leerPulsador() {
//   static bool estadoPrevio = HIGH; // Estado previo del botón
//   bool estadoActual = digitalRead(pinBoton);

//   // Detectar flanco descendente (presión del botón)
//   if (estadoPrevio == HIGH && estadoActual == LOW) {
//     contadorPulsaciones++;
//     Serial.print("Pulsaciones detectadas: ");
//     Serial.println(contadorPulsaciones);
//   }

//   estadoPrevio = estadoActual;

//   // Enviar contador al campo "status"
//   String status = "Pulsaciones: " + String(contadorPulsaciones);
//   ThingSpeak.setStatus(status);
// }

// Función para subir datos a ThingSpeak
void subirDatosThingSpeak() {
    float temperatura = dht.readTemperature();
    float humedad = dht.readHumidity();
    int estadoPote = analogRead(32);

    if (!isnan(temperatura) && !isnan(humedad)) {
        ThingSpeak.setField(1, temperatura);
        Serial.println("Temperatura: " + String(temperatura));
        ThingSpeak.setField(2, humedad);
        Serial.println("Humedad: " + String(humedad));
        ThingSpeak.setField(3, estadoPote);
        Serial.println("Potenciometro: " + String(estadoPote));
        ThingSpeak.setField(4, contadorPulsador); // Reflejar pulsaciones en ThingSpeak
        ThingSpeak.setStatus("Pulsador presionado " + String(contadorPulsador) + " veces");
        Serial.println("Pulsador: " + String(contadorPulsador));

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

// Función para manejar las solicitudes de clientes web
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
        delay(10); // Breve retraso antes de desconectar
        client.stop();
        Serial.println("Cliente desconectado");
    }
}


// Función para mostrar la página web con datos de ThingSpeak
void mostrarPaginaWeb(WiFiClient& client) {
    String datos = obtenerDatosThingSpeak();

    // Crear un buffer JSON para parsear los datos
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, datos);

    String contenidoHTML;

    if (error) {
        contenidoHTML = "<p class='error'>Error al parsear los datos de ThingSpeak.</p>";
    } else {
        // Extraer datos del JSON
        JsonObject channel = doc["channel"];
        JsonObject feed = doc["feeds"][0]; // Última entrada

        String canalNombre = channel["name"].as<String>();
        String canalDescripcion = channel["description"].as<String>();

        String temperatura = feed["field1"].as<String>();
        String humedad = feed["field2"].as<String>();
        String posicionPote = feed["field3"].as<String>();
        String pulsaciones = feed["field4"].as<String>();
        String fechaRegistro = feed["created_at"].as<String>();


        // Construir el contenido HTML con los datos extraídos
        contenidoHTML = "<h1>Datos obtenidos de ThingSpeak</h1>";
        contenidoHTML += "<p><strong>Nombre del Canal:</strong> " + canalNombre + "</p>";
        contenidoHTML += "<p><strong>Descripcion:</strong> " + canalDescripcion + "</p>";
        contenidoHTML += "<h2>Ultimos datos registrados</h2>";
        contenidoHTML += "<ul>";
        contenidoHTML += "<li><strong>Temperatura:</strong> " + temperatura + " °C</li>";
        contenidoHTML += "<li><strong>Humedad:</strong> " + humedad + " %</li>";
        contenidoHTML += "<li><strong>Posicion del Potenciometro:</strong> " + posicionPote + "</li>";
        contenidoHTML += "<li><strong>Pulsaciones del Boton:</strong> " + pulsaciones + "</li>";
        contenidoHTML += "<li><strong>Fecha de Registro:</strong> " + fechaRegistro + "</li>";
        contenidoHTML += "</ul>";
        
    }

    // Enviar la página al cliente
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println();

    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<title>Datos de ThingSpeak</title>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; background-color: #e8f4f8; color: #333; margin: 0; padding: 0; }");
    client.println(".container { max-width: 800px; margin: 30px auto; background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 4px 10px rgba(0, 0, 0, 0.1); }");
    client.println("h1 { color: #0078a0; text-align: center; }");
    client.println("h2 { color: #005f7a; margin-top: 20px; }");
    client.println("ul { display: flex; flex-direction: column; gap: 10px; list-style: none; padding: 0; }");
    client.println("li { background: #f0f8fc; margin: 10px 0; padding: 10px; border-left: 5px solid #0078a0; border-radius: 4px; }");
    client.println("strong { color: #005f7a; }");
    client.println(".error { color: #d9534f; font-weight: bold; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");
    client.println("<div class='container'>");
    client.println(contenidoHTML);
    client.println("</div>");
    client.println("</body>");
    client.println("</html>");

    client.println();
}

String ajustarHorario(String fechaUTC) {
    // Verificar que el formato tenga longitud válida
    if (fechaUTC.length() < 20) {
        return "Fecha inválida";
    }

    // Extraer la parte de la hora (HH)
    int horaUTC = fechaUTC.substring(11, 13).toInt();
    int horaCordoba = horaUTC - 3; // Ajustar a UTC-3

    // Manejar desbordamiento de horas
    if (horaCordoba < 0) {
        horaCordoba += 24;
    }

    // Reemplazar la hora en la cadena original
    String fechaAjustada = fechaUTC;
    String nuevaHora = (horaCordoba < 10) ? "0" + String(horaCordoba) : String(horaCordoba);
    fechaAjustada.replace(fechaUTC.substring(11, 13), nuevaHora);

    return fechaAjustada;
}


// Función para obtener datos de ThingSpeak
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
