#include <Audio.h>
#include "MyDsp.h"

#define POT_PIN A0  // Broche du potentiomètre

MyDsp myDsp;
AudioOutputI2S audioOut;
AudioControlSGTL5000 audioShield;

AudioConnection patchCord1(myDsp, 0, audioOut, 0); // Gauche
AudioConnection patchCord2(myDsp, 1, audioOut, 1); // Droite

float pan = 0.5;   // Valeur de panning initiale
float lastPan = 0.5; // Stocke la dernière valeur lue
float smoothingFactor = 0.1; // Coefficient pour lisser les variations


void setup() {
    AudioMemory(10);
    audioShield.enable();
    audioShield.volume(0.5);
}

void loop() {
    myDsp.setFreq(random(50,1000));
    int potValue = analogRead(POT_PIN);
    float newPan = potValue / 1023.0; // Normalisation entre 0 et 1

    // Appliquer un filtrage simple pour éviter les sauts brutaux
    pan = lastPan * (1.0 - smoothingFactor) + newPan * smoothingFactor;
    lastPan = pan; 

    myDsp.setPan(pan);

    delay(5);
}