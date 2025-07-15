/*Este archivo actualiza el codigo de la placa usando el la conexion grps 
  por lo general deberia ser llamada desde una funcion "actualizar_firmware()"
  y esta debe ejecutar la funcion "GprsHttpUpdate(host,Binario)"
  al importar la esta "libreria" debes hacerlo despues de declarar el cliente Wifi , y el modem GRPS

  este código no es necesario para los dispositivos que solo usan wifi, pero si es util para los que solo usan GPRS, 
  en ese caso hay validar antes de usar este codigo  sino utilizar la libreria <HTTPUPDATE> tradicional. 

*/

#include <Update.h>
//#include "FS.h"
#include "SPIFFS.h"
int port = 80;

void listDir(fs::FS &fs, const char *dirname, uint8_t levels){

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root){
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file){
    if (file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels){
        listDir(fs, file.name(), levels - 1);
      }
    }
    else{
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void performUpdate(Stream &updateSource, size_t updateSize){


 if  (Update.begin(updateSize)){
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize){
      Serial.println("Writes : " + String(written) + " successfully");
    }
    else{
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    if (Update.end()){
      Serial.println("OTA finished!");
      if (Update.isFinished()){
        Serial.println("Restart ESP device!");
        ESP.restart();
      }
      else{
        Serial.println("OTA not fiished");
      }
    }
    else{
      Serial.println("Error occured #: " + String(Update.getError()));
    }
  }
  else{
    Serial.println("Cannot beggin update");
  }
}

void updateFromFS(){
  File updateBin = SPIFFS.open("/update.bin");
  if (updateBin){
    if (updateBin.isDirectory()){
      Serial.println("Directory error");
      updateBin.close();
      return;
    }

    size_t updateSize = updateBin.size();

    if (updateSize > 0){
      Serial.println("Starting update");
      performUpdate(updateBin, updateSize);
    }
    else{
      Serial.println("Error, archivo vacío");
    }
    updateBin.close();
    //fs.remove("/update.bin");
  }
  else{
    Serial.println("no such binary");
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message){
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)){
    Serial.println("APOK");
  }
  else{
    Serial.println("APX");
  }
}

void readFile(fs::FS &fs, const char *path){
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file || file.isDirectory()){
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()){
    Serial.write(file.read());
    delayMicroseconds(100);
  }
}

void writeFile(fs::FS &fs, const char *path, const char *message){
  Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file){
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)){
    Serial.println("File written");
  }
  else{
    Serial.println("Write failed");
  }
}

void deleteFile(fs::FS &fs, const char *path){
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)){
    Serial.println("File deleted");
  }
  else{
    Serial.println("Delete failed");
  }
}

void printPercent(uint32_t readLength, uint32_t contentLength){
  // If we know the total length
  if (contentLength != -1){
    Serial.print("\r ");
    Serial.print((100.0 * readLength) / contentLength);
    Serial.print('%');
  }
  else{
    Serial.println(readLength);
  }
}

void GprsHttpUpdate(String host,String bin ){

   if (!SPIFFS.begin(true)){
    Serial.println("Fallo el montar SPIFFS");
    return;
  }
  SPIFFS.format();
  listDir(SPIFFS, "/", 0);

  if (gprsClient.connect(host.c_str(), port)) {
    Serial.println("contectandose a : " + String(host));
    //CONNEXION SATISFACTORIA
    Serial.println("Fetching Bin: " + String(bin));
      // OBTENIERNDO ARCHIVO BIN
    gprsClient.print(String("GET ") + bin + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Cache-Control: no-cache\r\n" +
                  "Connection: close\r\n\r\n");
    
    unsigned long timeout = millis();
    while (gprsClient.available() == 0) {
      if (millis() - timeout > 5000) {
        Serial.println("Client Timeout !");
        gprsClient.stop();
        return;
      }
    }
    Serial.println("Reading header");
    uint32_t contentLength;
    File file = SPIFFS.open("/update.bin", FILE_APPEND);
    while (gprsClient.available()) {
      String line = gprsClient.readStringUntil('\n');
      line.trim();
      Serial.println(line);    // Uncomment this to show response header
      line.toLowerCase();
      if (line.startsWith("content-length:")){
        contentLength = line.substring(line.lastIndexOf(':') + 1).toInt();
      }
      else if (line.length() == 0){
        break;
      }
    }
    timeout = millis();  
    uint32_t readLength = 0;
    unsigned long timeElapsed = millis();
    printPercent(readLength, contentLength);
    while (readLength < contentLength && gprsClient.connected() && millis() - timeout < 10000L){
      int i = 0;
      while (gprsClient.available()){
        // read file data to spiffs
        if (!file.print(char(gprsClient.read()))){
          //Serial.println("Appending file");
        }
        readLength++;
        if (readLength % (contentLength / 13) == 0){
        printPercent(readLength, contentLength);
        }
        timeout = millis();
      }
    }
    file.close();
    printPercent(readLength, contentLength);
    timeElapsed = millis() - timeElapsed;
    Serial.println();
    gprsClient.stop();
    Serial.println("stop client");
    modem.gprsDisconnect();
    Serial.println("gprs disconnect");
    Serial.println();
    float duration = float(timeElapsed) / 1000;
    updateFromFS();
  }

}

void sdLog(String mensaje){
  String mensaje_completo = FechaPantalla+" "+mensaje+"\n";  
  //writeFile(SD, "/logs.txt", mensaje_completo);
  appendFile(SD, NombreLog.c_str(), mensaje_completo.c_str());
}
//verifica que el archivo sea mas antigui de 31 dias y que sea un archivo de log y no otro tipo de archivo
void antiguedad(String log_name){
  int year, month, day;
  sscanf(log_name.c_str(), "/%d-%d-%d.txt", &year, &month, &day);  
  int logtimestamp=stamp.timestamp(year - 2000, month, day,0, 0,0);
    Serial.println(FechaEpoch-logtimestamp);
    //if(FechaEpoch-logtimestamp>2678400){//31 dias de diferencia 
    if(FechaEpoch-logtimestamp>86400){//1 dia de diferencia para probar 
      Serial.print("eliminar archivo");
      Serial.println(log_name);
      sdLog("eliminar archivo: "+log_name);
      deleteFile(SD,log_name.c_str());
    }
    Serial.print(logtimestamp);
  
}

void delete_logs_file(fs::FS &fs, const char *dirname, uint8_t levels){

  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root){
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()){
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file){
    if (file.isDirectory()){
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels){
        listDir(fs, file.name(), levels - 1);
      }
    }
    else{
      Serial.print("  FILE: ");
      antiguedad(file.name());
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}


void listado_de_logs(){
  sdLog("Delete logs");
  delete_logs_file(SD, "/", 0);
}
