
#include <Wire.h> 
#include "RTClib.h"        //Librería del reloj
#include <ArduinoJson.h>  //Librería para extraer datos Json
#include <SPI.h>          //Importa librería de comunicacion SPI
#include <Ethernet.h>     //Importa librería ethernet

RTC_DS1307 RTC;

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  //Asigna MAC 
byte ip[] = {192, 168, 1, 177 };                      //Asigna IP de la red local
byte gateway[] = {192, 168, 1, 1 };                   //Asigna puerta de enlace
byte subnet[] = {255, 255, 255, 0 };                  //Asigna mascara de subred

EthernetServer server(81);                            //Crea servidor web en el puerto 81
EthernetClient client;

/*
# Codigo de ejemplo para leer valores de un sensor de humedad de suelo
# Valores digitales
# 1 - Seco
# 0 - Mojado
*/
const int dh = 3; //Sensor de humedad de suelo, salida digital

//Gestion Reles
int rele1 = 5;
int rele2 = 6;
int rele3 = 7;
int rele4 = 8;
String estado="OFF";

//Hora programada de riego
int HoraRiego=22;
int MinutoRiego=30;

//Prevision meteorologica
String lluvia="Sin lluvia";

//Respuesta HTTP
const unsigned long HTTP_TIMEOUT = 20000;  // Tiempo maximo de respuesta del servidor
const size_t MAX_CONTENT_SIZE = 2512;       //Maximo tamaño de la respuesta HTTP

//riego
void setup()
{
pinMode(dh, INPUT);   //Se declara la variable dh que corresponde al sensor de humedad como variable de entrada
pinMode(rele1,OUTPUT);//Se declara la variable rele que corresponde al rele como variable de salida
pinMode(rele2,OUTPUT);
pinMode(rele3,OUTPUT);
pinMode(rele4,OUTPUT);
Wire.begin(); // Inicia el puerto I2C
RTC.begin(); // Inicia la comunicación con el RTC
RTC.adjust(DateTime(__DATE__, __TIME__)); // Establece la fecha y hora inicialmente o con el cambio de pila del reloj
Serial.begin(9600); // Establece la velocidad de datos del puerto serie
// Inicia la comunicación Ethernet y el servidor:
Ethernet.begin(mac,ip,gateway,gateway,subnet);
server.begin();
Serial.print("La IP del servidor es: ");
Serial.println(Ethernet.localIP());
}

void loop()
{
  //Obtiene la fecha y hora del RTC y se muestra por el Monitor Serial para verificar que es correcta  
  DateTime now = RTC.now(); // Obtiene la fecha y hora del RTC
  Serial.print(now.year(), DEC); // Año
  Serial.print('/');
  Serial.print(now.month(), DEC); // Mes
  Serial.print('/');
  Serial.print(now.day(), DEC); // Dia
  Serial.print(' ');
  Serial.print(now.hour(), DEC); // Horas
  Serial.print(':');
  Serial.print(now.minute(), DEC); // Minutos
  Serial.print(':');
  Serial.print(now.second(), DEC); // Segundos
  Serial.println();
  delay(1000); // La información se actualiza cada 1 seg.

  if (now.minute() == 0 && now.second() == 0) {
    peticionPrevision();
  }

  int humedad = digitalRead(dh); //Se lee el valor del sensor de humedad que puede ser 0 o 1

  EthernetClient client = server.available(); //crea un cliente web
  //Detecta un cliente a traves de una peticion HTTP
  if (client) {
    Serial.println("Nuevo cliente");
    boolean currentLineIsBlank = true; //Una petición http acaba con una linea en blanco
    String cadena="";                  //Crea una cadena de caracteres vacía
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();        //Lee la peticion HTTP caracter por caracter
        Serial.write(c);               //Visualiza la petición HTTP por el Monitor Serial
        cadena.concat(c);              //Une el String 'cadena' con la petición HTTP. De este modo se convierte la petición HTTP a un String
        //Al haber convertido la petición HTTP a una cadena de caracteres, se podría buscar partes del texto.
        int posicion=cadena.indexOf("RIEGO="); //Guarda la posición de la instancia "RIEGO=" a la variable 'posicion'
        if(cadena.substring(posicion)=="RIEGO=ON"){//Si a la posición 'posicion' hay "RIEGO=ON"
          ejecucion();
          estado="ON";
        }
        if(cadena.substring(posicion)=="RIEGO=1"){//Si a la posición 'posicion' hay "RIEGO=ON"
          estado="ON1";
          ejecucion(); 
        }
        if(cadena.substring(posicion)=="RIEGO=2"){//Si a la posición 'posicion' hay "RIEGO=ON"
          estado="ON2";
          ejecucion();
        }
        if(cadena.substring(posicion)=="RIEGO=3"){//Si a la posición 'posicion' hay "RIEGO=ON"
          estado="ON3";
          ejecucion();
        }
        if(cadena.substring(posicion)=="RIEGO=OFF"){//Si a la posición 'posicion' hay "RIEGO=OFF"
          apagado();
          estado="OFF";
        }
        //Cuando haya llegado al final de la linea y reciba una línea en blanco, quiere decir que la petición HTTP ha acabado y el servidor Web está listo para enviar una respuesta
        if (c == '\n' && currentLineIsBlank) {
          // Envía una respuesta HTTP con una cabecera estandar
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");  
	  client.println("Refresh: 30");  // Actualiza la pagina cada 30 segundos
          client.println();
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
          client.print("HORA: ");
          client.print(now.hour(), DEC);
          client.print(":");
          client.print(now.minute(), DEC);
          client.print(":");
          client.print(now.second(), DEC);
          client.print("<br />");
          client.println("<h1 align='center'>RIEGO</h1><h3 align='center'>Sistema de riego automatico con control remoto</h3>");
          //Crea los botones. Para enviar parametros a través de HTML se utiliza la URL. Los parámetros se envian a través del símbolo '?'
          client.println("<div style='text-align:center;'>");
          client.println("<button onClick=location.href='./?RIEGO=ON\' style='margin:auto;background-color: #84B9FF;color: snow;padding: 12px;border: 1px solid #3F7CFF;width:90px;'>");
          client.println("Programa");
          client.println("</button>");
          client.println("<button onClick=location.href='./?RIEGO=1\' style='margin:auto;background-color: #84B9FF;color: snow;padding: 12px;border: 1px solid #3F7CFF;width:90px;'>");
          client.println("Sector 1");
          client.println("</button>");
          client.println("<button onClick=location.href='./?RIEGO=2\' style='margin:auto;background-color: #84B9FF;color: snow;padding: 12px;border: 1px solid #3F7CFF;width:90px;'>");
          client.println("Sector 2");
          client.println("</button>");
          client.println("<button onClick=location.href='./?RIEGO=3\' style='margin:auto;background-color: #84B9FF;color: snow;padding: 12px;border: 1px solid #3F7CFF;width:90px;'>");
          client.println("Sector 3");
          client.println("</button>");
          client.println("<button onClick=location.href='./?RIEGO=OFF\' style='margin:auto;background-color: #84B9FF;color: snow;padding: 12px;border: 1px solid #3F7CFF;width:90px;'>");
          client.println("Apagado");
          client.println("</button>");
          client.println("<br /><br />");
          client.println("<b>RIEGO = ");
          client.print(estado);
          client.println("<br /><br />");
          client.println("<b>PREVISION = ");
          client.print(lluvia);
          client.println("</b><br />");
          if (humedad == LOW){
             client.println("No hace falta regar");   
          }
          client.println("<br />");       
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          //Se trata de una linea en blanco
          currentLineIsBlank = true;
        } 
        else if (c != '\r') {
          //Es un caracter mas
          currentLineIsBlank = false;
        }
      }
    }
    //Da tiempo al navegador para recibir todos los datos
    delay(10);
    // Cierra la conexión
    client.stop();
    Serial.println("Cliente desconectado");
  }
  //FIN SERVIDOR WEB

  if(now.hour() == HoraRiego & now.minute() == MinutoRiego){  
    if (humedad == LOW || lluvia == "rain"){
      Serial.println("No hace falta regar");   
    }
    else{
      ejecucion();
      Serial.println(lluvia);
    }
  }
}

void ejecucion() {
/*Nota esta el relé conectado como Normalmente Abierto
así solo se activará la carga cuando activemos la bobina
del relé, para que funcione al revés cambiaremos el cable
a la posición Normalmente Cerrado*/
if (estado == "ON1"){
  digitalWrite(rele1,HIGH);           // Se enciende el rele 1 correponde a la bomba de agua
  digitalWrite(rele2,HIGH);           // Se enciende el rele 2
  digitalWrite(rele3,LOW);            // Si estuviera encendido se apaga
  digitalWrite(rele4,LOW);            // Si estuviera encendido se apaga
}else{
  if (estado == "ON2"){
    digitalWrite(rele1,HIGH);           // Se enciende el rele 1 correponde a la bomba de agua
    digitalWrite(rele3,HIGH);           // Se enciende el rele 3
    digitalWrite(rele2,LOW);            // Si estuviera encendido se apaga
    digitalWrite(rele4,LOW);            // Si estuviera encendido se apaga
  }else{
    if (estado == "ON3"){
      digitalWrite(rele1,HIGH);           // Se enciende el rele 1 correponde a la bomba de agua
      digitalWrite(rele4,HIGH);           // Se enciende el rele 4
      digitalWrite(rele2,LOW);            // Si estuviera encendido se apaga
      digitalWrite(rele3,LOW);            // Si estuviera encendido se apaga
    }else{
      digitalWrite(rele1,HIGH);           // Se enciende el rele 1 correponde a la bomba de agua
      digitalWrite(rele2,HIGH);           // Se enciende el rele 2
      delay(9000000);  
      digitalWrite(rele2,LOW);          // Se apaga el rele 2
      digitalWrite(rele3,HIGH);           // Se enciende el rele 3
      delay(9000000);  
      digitalWrite(rele3,LOW);          // Se apaga el rele 3
      digitalWrite(rele4,HIGH);           // Se enciende el relay 4
      delay(9000000);                        // Se espera 600 segundos 
      digitalWrite(rele4,LOW);          // Se apaga el relay 3
      digitalWrite(rele1,LOW);          // Se apaga el relay 1 correponde a la bomba de agua
      delay(10000);              //durante 1 segundo
      estado="OFF";
    }
  }
}
}

void apagado() {
//Nota tenemos el relé conectado como Normalmente Abierto
//así solo se activará la carga cuando activemos la bobina
//del relé, para que funcione al revés cambiaremos el cable
//a la posición Normalmente Cerrado
digitalWrite(rele1,LOW);   //Desactiva todos los reles
digitalWrite(rele2,LOW); 
digitalWrite(rele3,LOW); 
digitalWrite(rele4,LOW); 
delay(10000);             //Espera durante 10 segundos 
estado="OFF";
}

void peticionPrevision() //Función cliente para recibir datos GET de un servidor externo
{
  if (client.connect("api.openweathermap.org", 80)) {
    Serial.println("Conectando...");
    client.println("GET /data/2.5/forecast?id=3118532&mode=json&units=metric&cnt=2&APPID=be2ab44c3659c1850800ca1b0a18ef1e HTTP/1.1");
    client.println("Host: api.openweathermap.org");
    client.println("Conexion: Respuesta terminada");
    client.println();
    //Las cabeceras web finalizan con una linea en blanco
    char finCabecera[] = "\r\n\r\n";
    client.setTimeout(HTTP_TIMEOUT);
    bool ok = client.find(finCabecera);
    if (!ok) {
      Serial.println("No hay respuesta o la respuesta no es correcta");
    }
    char respuesta[MAX_CONTENT_SIZE]; 
    size_t length = client.readBytes(respuesta, sizeof(respuesta));
    respuesta[length] = 0;
    Serial.println(respuesta);
    // Asigna el tamaño adecuado al buffer de forma dinamica.
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(respuesta);
    if (!root.success()) {
      Serial.println("Tranferencia JSON fallida!");
    }
    JsonArray& list = root["list"];
    JsonObject& ahora = list[0];
    JsonObject& manana = list[1];
    String tiempoActual = ahora["weather"][0]["description"];
    String prevision = manana["weather"][0]["description"];
    Serial.println(prevision);
    if ( prevision == "rain" || tiempoActual == "rain" ){
       lluvia = "rain";   
    }
    Serial.print("Prevision = ");
    Serial.println(prevision);
  }
}




