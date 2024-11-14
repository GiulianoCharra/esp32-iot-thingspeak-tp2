# esp32-iot-thingspeak-tp2

Proyecto IoT con ESP32 que envía datos de un sensor DHT22, potenciómetro y pulsador a ThingSpeak cada 20 segundos. Muestra temperatura, humedad, posición del potenciómetro y conteo de pulsaciones. Incluye una web para visualizar estos datos y una fuente externa desde ThingSpeak.

Trabajo práctico 2 de IoT
Deberá presentarse un sketch con las siguientes funcionalidades:

1. Conectar la ESP32 a una red Wifi.
2. Subir información a la nube mediante ThingSpeak cada 20 segundos. Dicha información será:
   - Lecturas de humedad y temperatura del sensor DHT22 de la placa de la cátedra.
   - Posición del potenciómetro de la placa de la cátedra.
   - En el campo “status” indicar la cantidad de veces que se ha presionado el pulsador
     desde el momento de encendido de la placa.
3. Mostrar en una página web, los datos subidos a la nube en el punto anterior, leyéndolos desde
   la nube (como si otro dispositivo lo hubiera subido anteriormente), más alguna lectura tomada
   de un canal público de ThingSpeak u otro servicio de clouding.

## Requisitos

- ESP32
- Sensor DHT22
- Potenciómetro
- Pulsador
- Conexión a Internet
- Cuenta en ThingSpeak

## Instalación

1. Clonar el repositorio:

```sh
git clone https://github.com/usuario/esp32-iot-thingspeak-tp2.git
```

2. Abrir el proyecto en el IDE de Arduino.
3. Instalar las siguientes librerías desde el gestor de librerías del IDE de Arduino:

- `WiFi.h`
- `DHT.h`
- `ThingSpeak.h`

4. Configurar las credenciales de la red WiFi y la clave de API de ThingSpeak en el archivo `config.h`:

```cpp
#define SECRET_SSID "yourSSID"
#define SECRET_PASS "yourPassword"
#define SECRET_CH_ID yourChannelID
#define SECRET_WRITE_APIKEY "yourWriteAPIKey"
```

5. Subir el sketch a la ESP32.

## Uso

1. Conectar la ESP32 a la red WiFi configurada.
2. La ESP32 comenzará a enviar datos a ThingSpeak cada 20 segundos.
3. Acceder a la página web generada por la ESP32 para visualizar los datos en tiempo real.

## Contribuciones

Las contribuciones son bienvenidas. Por favor, abre un issue o un pull request para discutir cualquier cambio.

## Licencia

Este proyecto está bajo la licencia MIT. Consulta el archivo `LICENSE` para más detalles.

## Autores

- Giuliano Benicio Charra Marquez
- Nombre del autor 2

## Agradecimientos

Agradecemos a la cátedra por proporcionar el material y el soporte necesario para la realización de este proyecto.
