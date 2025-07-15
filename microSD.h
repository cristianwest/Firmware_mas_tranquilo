//------------------------------------ LIBRERIAS ----------------------------------
#include <SD.h>
#include <FS.h>

//------------------------------------ SD -----------------------------------------
//#ifdef ENABLE_SPI_SDCARD
#define MY_CS       25
#define MY_SCLK     18
#define MY_MISO     19
#define MY_MOSI     33

void sd_card() { //Inicialización de tarjeta SD
  SPI.begin(MY_SCLK, MY_MISO, MY_MOSI);
  while (!SD.begin(MY_CS)) {
    Serial.println("Falló el montaje de la Tarjeta");
  }
  Serial.println("Tarjeta SD inicializada correctamente");
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("Tarjeta SD no Añadida");
    return;
  }
  Serial.print("SD Card Type: "); //Muestra la categoría de la tarjeta
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("DESCONOCIDA");
  }
}
