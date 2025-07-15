#include "QuickMedianLib.h" //Libreria de calculo rapido de mediana en arrays para filtrar lecturas con ruido

//----------------------------- Variables de estado --------------------------------
boolean balizaState = false; //variable para hacer controlar baliza [true,false]
boolean sirenaState = false;//variable para hacer sonar las sirenas [true,false]
boolean alarmaState = false;//variable para hacer sonar las sirenas [true,false]
int horaDeReinicio =10;//variable para almacenar la hora de reinicio predeterminada
String estado;//variable que almacena el estado de la alarma [00,01,10,00,FF],[sirena no funciona,sirena y baliza no funcionan,baliza no funciona,Sin bateria o bateria muerta]
String idUser = "";//se envia en un rabbit pero ya no se ocupa 
String alarmState = "";
boolean sirenaTest = false;
const int Ts = 200, battSamples = 9; //Cantidad de muestras de los sensores
int lumenes[Ts], decibeles[Ts], buzzer = 0, luz = 0, Sample = 0;
float bateria = 0, valueBatt = 0, battValue[battSamples];
byte flag1 = 0, flag2 = 0, battCount = 0, badCount = 0;

//-------------Pines Generales-------------------------------------------------------
#define pinSirena2 32
#define pinSirena1 0 //15
#define pinBaliza 2 //pin15 en conflicto al bootear pin2
#define pinLDR 35
#define pinMic 12
#define pinBATT 34
//-----------------------Altavoces------------------------------------------------
void sirena(String flag) {
  if (flag == "encender") {
    Serial.println("Sirena encendida");
    // aqui se cambia el estado de sirenaState, para que otra funcion haga el  encendido y apagado intermitente ver (alarmaLoop)
    sirenaState = true;
  }
  else if (flag == "apagar") {
    Serial.println("Sirena apagada");
    //en la logica sirenaState = falso  es la variable para apagar las sirenas, pero  como los relais apagan con 1   hay que escribir true en los pines que controlan los relays.
    sirenaState = false;
    digitalWrite(pinSirena1, !sirenaState);
    digitalWrite(pinSirena2, !sirenaState);
  }
}
//------------BALIZA--------------------------------
void baliza(String flag) {
  if (flag == "encender") {
    Serial.println("Baliza Encendida");
    balizaState =  true;
  }
  else if (flag == "apagar") {
    Serial.println("Baliza apagada");
    balizaState = false;
  }
  digitalWrite(pinBaliza, !balizaState);
}
//------------------SENSORES-------------------------------------------------------
void EstadoComponentes(byte flag) {
  //----------TOMA DE MUESTRAS  - SIRENA Y BALIZA------------------
  if (flag == 0) {
    if (Sample < Ts - 1) {
      lumenes[Sample] = analogRead(pinLDR);
      decibeles[Sample] = analogRead(pinMic);
      Sample++;
    }
  }

  //---------- BÚSQUEDA DE VALORES UMBRALES ----------------------
  else if (flag == 1) {
    //Busqueda de datos peak
    for (int k = 0; k < Ts - 1; k++) {
      //-----Baliza-----------
      if (lumenes[k] > 3500 && flag1 == 0) { //umbral de 0 a 4095
        flag1 = 1;
        luz = 1;
      }
      else if (flag1 == 0) {
        luz = 2;
      }
      //-----Sirena------------
      if (decibeles[k] > 3500 && flag2 == 0) { //umbral de 0 a 4095
        flag2 = 1;
        buzzer = 1;
      }
      else if (flag2 == 0) {
        buzzer = 2;
      }
    }
    //--------REPORTE---------
    if (luz == 1 && buzzer == 1) {
      estado = "11";  //"ok";
    }
    else if (luz == 2 && buzzer == 1) {
      estado = "10";  //"sirena no funciona";
    }
    else if (luz == 1 && buzzer == 2) {
      estado = "01";  //"baliza no funciona";
    }
    else if (luz == 2 && buzzer == 2) {
      estado = "00";  //"sirena y baliza no funcionan";
    }
    memset(lumenes, '0', Ts); memset(decibeles, '0', Ts);
    Sample = 0;
  }

  //--------------------------SENSOR BATERIA -----------------------
  else if (flag == 2) {
    valueBatt = map(analogRead(pinBATT), 2130, 2440, 0, 100);//Porcentaje 0 a 100. normal 6/5 = 1, 6/5.0 becomes 1.2 (double value), 6/5.0f becomes 1.2 (float value)2480

    // Verifica si el valor mapeado está por encima del 100%
    if (valueBatt > 100) {
        valueBatt = 100; // Establece el valor al 100% si está por encima
    }
    
    if (valueBatt<0){
      valueBatt=0;
    }
    bateria=valueBatt;
    if (valueBatt > 0) {
      if (battCount < battSamples) {
        battValue[battCount] = valueBatt;
        battCount++;
        badCount = 0;
      }
      else {
        bateria = QuickMedian<float>::GetMedian(battValue, battSamples);
        battCount = 0;
      }
    }
    else if (badCount == 10000) {
      bateria = 0;
      estado = "FF"; //Sin bateria o bateria muerta
    }
    else {
      badCount++;
    }
  }
}
//---------------------------LOOP----------------------------------
void AlarmaLoop() {
 if (sirenaState ||sirenaTest) {
    digitalWrite(pinSirena1, sirenaState);
    digitalWrite(pinSirena2, !sirenaState);
      delay(1000);
    digitalWrite(pinSirena1, true);
    digitalWrite(pinSirena2, true);
      delay(100); 
    digitalWrite(pinSirena1, !sirenaState);
    digitalWrite(pinSirena2, sirenaState);
      delay(1000);
    digitalWrite(pinSirena1, true);
    digitalWrite(pinSirena2, true);
      delay(100);   
    }
}
