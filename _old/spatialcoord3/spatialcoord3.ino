#include <Audio.h>
#include "MyDsp.h"

// Définition des objets audio
AudioInputI2S        audioInput;     // Entrée audio I2S (depuis le shield audio)
MyDsp myDsp;
AudioOutputI2S       audioOutput;    // Sortie audio I2S (vers casque ou haut-parleurs)
AudioConnection      patchCord1(myDsp, 0, audioOutput, 0);
AudioConnection      patchCord2(myDsp, 1, audioOutput, 1);
AudioControlSGTL5000 audioShield;   // Contrôle du chip audio

void setup() {
    AudioMemory(12);
    audioShield.enable();
    audioShield.volume(0.2);  // Volume global
}

void loop() {
  myDsp.setFreq(1000);
  int potX = analogRead(A0);
  int potY = analogRead(A2);
  float pan = potX / 1023.0;       // Valeur entre 0.0 (gauche) et 1.0 (droite)
  float distance = potY / 1023.0;  // Valeur entre 0.0 (proche) et 1.0 (lointaine)
  myDsp.setPosition(pan, distance);
  delay(300);
}
