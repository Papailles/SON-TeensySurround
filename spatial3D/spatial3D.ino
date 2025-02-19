#include <Audio.h>
#include "MyDsp.h"

// Création des objets audio du système Teensy Audio
AudioOutputI2S        audioOutput;                   // Sortie audio (vers le shield)
MyDsp                 myDsp;                         // Votre DSP 3D
AudioConnection       patchCord1(myDsp, 0, audioOutput, 0);
AudioConnection       patchCord2(myDsp, 1, audioOutput, 1);
AudioControlSGTL5000  audioShield;                   // Contrôle du shield audio

// Définition des pins pour les potentiomètres
// - A0 pour le panning horizontal (x, 0.0 = gauche, 1.0 = droite)
// - A1 pour la profondeur (y, -1.0 = derrière, 1.0 = devant)
// - A2 pour la hauteur (z, -1.0 = en dessous, 1.0 = au-dessus)
const int potXPin = A0;
const int potYPin = A2;
const int potZPin = A3;

void setup() {
  Serial.begin(9600);
  AudioMemory(12);
  audioShield.enable();
  audioShield.volume(0.3);

  // Optionnel : définir une fréquence de base (par exemple 440 Hz)
  myDsp.setFreq(440.0);
}

void loop() {
  // Lecture des potentiomètres
  int rawX = analogRead(potXPin);
  int rawY = analogRead(potYPin);
  int rawZ = analogRead(potZPin);

  // Mapping des valeurs lues aux plages attendues :
  // x de 0 à 1023 -> 0.0 à 1.0
  float posX = rawX / 1023.0;
  // y de 0 à 1023 -> -1.0 à 1.0 (1 = devant, -1 = derrière)
  float posY = (rawY / 1023.0) * 2.0 - 1.0;
  // z de 0 à 1023 -> -1.0 à 1.0 (1 = au-dessus, -1 = en dessous)
  float posZ = (rawZ / 1023.0) * 2.0 - 1.0;

  // Envoi des positions au DSP 3D
  myDsp.setPosition(posX, posY, posZ);

  // Debug (optionnel)
  Serial.print("x: ");
  Serial.print(posX, 2);
  Serial.print(" | y: ");
  Serial.print(posY, 2);
  Serial.print(" | z: ");
  Serial.println(posZ, 2);

  delay(300);
}
