#include <Audio.h>
#include "MyDsp.h"

// Définition des objets audio
AudioInputI2S        audioInput;     // Entrée audio I2S (depuis le shield audio)
MyDsp myDsp;
AudioOutputI2S       audioOutput;    // Sortie audio I2S (vers casque ou haut-parleurs)
AudioConnection      patchCord1(myDsp, 0, audioOutput, 0);
AudioConnection      patchCord2(myDsp, 1, audioOutput, 1);
AudioControlSGTL5000 audioShield;   // Contrôle du chip audio

const int potPin = A0;  // Potentiomètre sur A0

void setup() {
    AudioMemory(12);
    audioShield.enable();
    audioShield.volume(0.2);  // Volume global
}

void loop() {
  myDsp.setFreq(1000);
  int potValue = analogRead(potPin);
  float pan = potValue / 1023.0;  // Valeur comprise entre 0.0 et 1.0
  myDsp.setPosition(pan, 0);      // Met à jour le panoramique
  delay(300);
}
