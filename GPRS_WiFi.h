//---------------------------------- LIBRERIAS -------------------------------------
#include <WiFi.h>
#include <Wire.h>
#include <timestamp32bits.h>     //Tiempo Epoch

#define SerialMon Serial
#define SerialAT Serial1
#define TINY_GSM_RX_BUFFER  1024   // Set RX buffer to 1Kb
#define TINY_GSM_MODEM_SIM7600   //Define modem utilizado
#include <TinyGsmClient.h>          //Libreria GSM/GPRS
#include <TimeLib.h>

unsigned long tiempo = 0;
unsigned long periodo2 = -2000;
char jsonChar[1000];                //Almacena json para enviar a rabbit
int DatosNoEnviados = 0;   
String FechaPantalla = "";
String NombreLog="";
//--------------------------------- Clientes Internet -------------------------------
TinyGsm modem(SerialAT);
TinyGsmClient gprsClient(modem);
WiFiClient wifiClient;

//--------------------------------- TTGO T-Call pins --------------------------------
// Pines para usar SimCard. Sin estos no se puede usar el modulo ni bateria.
//#define MODEM_RST            5
//define MODEM_PWKEY          4
//#define MODEM_RESET          4
//define MODEM_POWER_ON       4
#define MODEM_TX             13
#define MODEM_RX             14

//----------------------------------- FECHA ----------------------------------------
int FechaEpoch;
timestamp32bits stamp = timestamp32bits();
int Minutes, Hour, Year, Seconds;
float timezone = 0;

void LocalTime() {
  int Month, Day;
  modem.getNetworkTime(&Year, &Month, &Day, &Hour, &Minutes, &Seconds, &timezone); //Obtiene la fecha y hora
  FechaPantalla = String(Day) + "/" + String(Month) + "/" + String(Year) + " " + String(Hour) + ":" + String(Minutes) + ":" + String(Seconds);
  NombreLog = "/"+String(Year) + "-" + String(Month) + "-" + String(Day)+".txt";
  // CONVERSION A EPOCH TIME
  int DifEpoch = -3600 * (int)timezone;
  FechaEpoch = stamp.timestamp(Year - 2000, Month, Day, Hour, Minutes, Seconds);  //transformar la fecha hora desde YYYY/MM/DD HH:MM:SS a epoch time // Tiempo según zona horaria de la simcard
  FechaEpoch = FechaEpoch + DifEpoch; //Traspasa el tiempo a seg en GMT-0
  //Hora invierno (GMT-4) primer sabado de abril y termina el primer sabado de septiembre (GMT-3)
  if (weekday(FechaEpoch) == 7 && Minutes == 0 && ( (int)Month >= 4 && (int)Month < 9 ) && (int)timezone == 3) {
    ESP.restart();
  }
  else if (weekday(FechaEpoch) == 7 && Minutes == 0 && ( (int)Month >= 9 || (int)Month < 4) && (int)timezone == 4) {
    ESP.restart();
  }
}

int RST_ESP = 0;

//---------------------------------- APN ------------------------------------------
const char apn[]      = "m2m.entel.cl";
const char gprsUser[] =  "entelpcs";
const char gprsPass[] = "entelpcs";

void ConexionAPN() {
  if (modem.isGprsConnected() != 1) {
    Serial.print("\nConectando a APN ");
    Serial.print(apn);
    Serial.print("...");
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
      Serial.println(" No se pudo conectar al APN");
    }
    else {
      Serial.println(" Conexion OK");
    }
  }
}

//--------------------------- Intensidad de la Señal ------------------------------
int dBm() { //Muestra el nivel de la señal en dBm
  int csq = modem.getSignalQuality();
  return ((csq * 2) - 113);
}
