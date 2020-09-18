## Agentes de la Calidad del Aire (AQAV80)
Cuando se desea profundizar en el problema de la calidad del aire en Medellín nos encontramos con varios obstáculos debido a la complejidad del problema y la limitada capacidad de los ciudadanos para recolectar y examinar las mediciones.
El Kit AQA se esfuerza por acercar al ciudadano a la posibilidad de medir y analizar el aire que respira. De hecho, en las actividades propuestas se aprende a ensamblar un dispositivo para la medición del materialparticulado, y se considera unas herramientas para el estudio de las mediciones.
Es en los detalles del ensamblaje y el uso de las herramientas de análisis donde se puede pensar con las manos y entrar en relación directa con el problema de la calidad del aire.

## Instalación

descargar repositorio

          git clone https://github.com/jero98772/aqav80.git

https://comunidad.unloquer.org/t/como-programar-el-aqa/33




## Enlaces
  * [Ejemplo de la trama de datos](https://raw.githubusercontent.com/daquina-io/VizCalidadAire/master/data/points.csv) https://raw.githubusercontent.com/daquina-io/VizCalidadAire/master/data/points.csv
  * [Foro](https://comunidad.unloquer.org/) comunidad unloquer  https://comunidad.unloquer.org/
  * [wiki](http://wiki.unloquer.org/personas/brolin/proyectos/agentes_calidad_aire) wiki unloquer http://wiki.unloquer.org/personas/brolin/proyectos/agentes_calidad_aire
  * [mapa de mediciones](http://daquina.io/aqaviz/) daquia.io http://daquina.io/aqaviz/
  * [Licensed under the TAPR Open Hardware License](www.tapr.org/OHL): www.tapr.org/OHL

## Como leer los colores
el color gradiente de los leds funciona mientras mas intenso el color mas contaminacion  y menos inteso el color  menos contaminacion

el color gradiente de los leds funciona mientras mas intenso el color mas contaminacion y menos inteso el color menos contaminacion 

color > 13 resultado genrlamente verde 

13 < color < 35 resultado  generamente amarillo

35 < color < 55 resultado  generalmente naranja

55 < color < 75 resultado  generalmente rojo

75 < color < 255 resultado  generalmente morado

color < 255 marron

## ¿De donde vienen los datos? -> de estos sensores: 

 * dht11 --> temperatura y humedad 
 * plantower --> material particulado
 * max9814 --> ruido

## ¿Hacia donde salen los datos?

  * [mapa de mediciones](http://daquina.io/aqaviz/) daquia.io http://daquina.io/aqaviz/
  * [los dash board](http://aqa.unloquer.org:8888/sources/2/chronograf/data-explorer) chronograf http://aqa.unloquer.org:8888/sources/2/chronograf/data-explorer
  en la parte de  v80.autogen


## ¿Como lo instalo? 

Usando Visual Studio Code, aka Code en sistemas GNU/Linux. Es tan facil como instalar Platformio y asegurarse que en el archivo platformio.ini se nombre el dispositivo que queremos flashear:  upload_port = /dev/ttyUSB0  

En el archivo main.ccp se deben cambiar los valores para ubicacion y datos de la red que se utilizara para enviar los datos capturados, por ejemplo:

#define FIXED_LAT "6.167222"
#define FIXED_LON "-75.426667"
#define SENSOR_ID "Rionegro"

y, mas abajo en el mismo archivo:

#define WIFI_SSID "ESSID/Nombre de la red"
#define WIFI_PASS "PASSWORD/Clave de la red"

Para otras maneras de instalarlo lea aqui:  
[enlace a como instalarlo en comunidad unloquer](https://comunidad.unloquer.org/t/cargar-el-firmware-desde-linea-de-comando/118) 

## Contacto 

 * irc --> #un/loquer
 * twitter --> [twitter de unloquer](https://twitter.com/unloquer?lang=es) enlace a https://twitter.com/unloquer?lang=es
