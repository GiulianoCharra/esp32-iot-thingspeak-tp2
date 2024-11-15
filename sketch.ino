#include <WiFi.h>
#include <HTTPClient.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Adafruit_SH110X.h>

// Credenciales de WiFi
const char* ssid = "Tu_SSID";  // Nombre de la red WiFi
const char* password = "Tu_PASSWORD";  // Contraseña de la red WiFi

// Configuración de ThingSpeak
unsigned long channelID = 2745994; // ID del canal en ThingSpeak
const char* WriteAPIKey = "12KRBEBJB75FKDBN";  // Clave de escritura para el canal
const String readAPIKey = "TU_READ_API_KEY";  // Clave de lectura del canal
const String server = "http://api.thingspeak.com";  // URL base de ThingSpeak

// Pines y objetos
DHT dht(33, DHT22);  // Objeto DHT para medir temperatura y humedad, conectado al pin 33
WiFiClient cliente;  // Cliente WiFi para enviar y recibir datos
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);  // Objeto para la pantalla OLED
WiFiServer serverWeb(80);  // Servidor web en el puerto 80

// Variables globales
int contadorPulsador = 0;  // Contador de pulsaciones del botón
unsigned long lastTime = 0;  // Marca de tiempo para la última subida de datos
const unsigned long intervalo = 20000;  // Intervalo de tiempo para subir datos (20 segundos)

// Función para conectar la ESP32 a la red WiFi
void conectarWifi() {
    Serial.begin(115200);  // Inicializar la comunicación serial
    WiFi.begin(ssid, password);  // Conectar a la red WiFi
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {  // Esperar hasta que la conexión se complete
        delay(500);
        Serial.print(".");  // Mostrar puntos en la consola mientras espera la conexión
    }
    Serial.println("\nWiFi conectado");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());  // Mostrar la dirección IP asignada
}

// Configuración inicial de la ESP32
void setup() {
    conectarWifi();  // Llamar a la función para conectar al WiFi
    ThingSpeak.begin(cliente);  // Inicializar la comunicación con ThingSpeak
    dht.begin();  // Iniciar el sensor DHT22
    pinMode(4, INPUT_PULLUP);  // Configurar el pin 4 como entrada con resistencia de pull-up para el pulsador
    attachInterrupt(digitalPinToInterrupt(4), contarPulsaciones, FALLING);  // Configurar la interrupción para el pulsador

    // Configuración de la pantalla OLED
    display.begin(0x3c, true);  // Iniciar la pantalla OLED en la dirección I2C 0x3C
    display.setTextSize(1);  // Tamaño del texto en la pantalla
    display.setTextColor(SH110X_WHITE);  // Color del texto

    // Iniciar el servidor web
    serverWeb.begin();
    Serial.println("Servidor web iniciado.");
}

// Bucle principal
void loop() {
    // Verificar si ha pasado el intervalo de tiempo para subir datos
    if (millis() - lastTime > intervalo) {
        subirDatosThingSpeak();  // Llamar a la función para subir datos a ThingSpeak
        lastTime = millis();  // Actualizar la marca de tiempo
    }
    manejarClienteWeb();  // Manejar cualquier cliente conectado al servidor web
}

// Función de interrupción para contar las pulsaciones del botón
void contarPulsaciones() {
    static unsigned long ultimoTiempoPulsacion = 0;  // Marca de tiempo de la última pulsación
    const unsigned long debounce = 200;  // Tiempo de debounce (200 ms)

    // Verificar si ha pasado el tiempo de debounce para contar una nueva pulsación
    if (millis() - ultimoTiempoPulsacion > debounce) {
        contadorPulsador++;  // Incrementar el contador de pulsaciones
        ultimoTiempoPulsacion = millis();  // Actualizar la marca de tiempo
    }
}

// Función para subir datos a ThingSpeak
void subirDatosThingSpeak() {
    float temperatura = dht.readTemperature();  // Leer la temperatura del sensor
    float humedad = dht.readHumidity();  // Leer la humedad del sensor
    int estadoPote = analogRead(32);  // Leer la posición del potenciómetro en el pin 32

    // Verificar que las lecturas del DHT no sean inválidas
    if (!isnan(temperatura) && !isnan(humedad)) {
        // Establecer los campos de ThingSpeak con los valores leídos
        ThingSpeak.setField(1, temperatura);
        ThingSpeak.setField(2, humedad);
        ThingSpeak.setField(3, estadoPote);
        // Establecer el campo de estado con la cantidad de veces que se presionó el pulsador
        ThingSpeak.setStatus("Pulsador presionado " + String(contadorPulsador) + " veces");

        // Enviar los datos a ThingSpeak y verificar la respuesta
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
    WiFiClient client = serverWeb.available();  // Verificar si hay un cliente conectado
    if (client) {
        Serial.println("Cliente conectado");
        String header;  // Variable para almacenar el encabezado de la solicitud
        bool requestHandled = false;  // Bandera para indicar si la solicitud ha sido procesada

        // Leer la solicitud del cliente
        while (client.connected() && !requestHandled) {
            if (client.available()) {
                char c = client.read();  // Leer un byte de la solicitud
                header += c;
                if (c == '\n' && header.endsWith("\r\n\r\n")) {
                    requestHandled = true;  // Indicar que la solicitud ha sido procesada
                    mostrarPaginaWeb(client);  // Mostrar la página web al cliente
                }
            }
        }
        client.stop();  // Cerrar la conexión con el cliente
        Serial.println("Cliente desconectado");
    }
}

// Función para mostrar la página web con datos de ThingSpeak
void mostrarPaginaWeb(WiFiClient& client) {
  String datos = obtenerDatosThingSpeak();  // Obtener los datos de ThingSpeak

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

// Función para obtener datos de ThingSpeak
String obtenerDatosThingSpeak() {
    HTTPClient http;  // Objeto HTTP para realizar solicitudes
    // URL para obtener los datos de ThingSpeak
    String url = server + "/channels/" + String(channelID) + "/feeds.json?api_key=" + readAPIKey + "&results=1";
    http.begin(url);  // Iniciar la solicitud HTTP

    int httpResponseCode = http.GET();  // Enviar la solicitud GET y guardar la respuesta
    String payload;

    if (httpResponseCode > 0) {  // Verificar si la respuesta es válida
        payload = http.getString();  // Obtener la respuesta como una cadena
        Serial.println("Datos obtenidos correctamente");
    } else {
        payload = "Error al obtener datos: " + String(httpResponseCode);  // Mensaje de error
        Serial.println(payload);
    }

    http.end();  // Finalizar la conexión HTTP
    return payload;  // Devolver la respuesta obtenida
}
