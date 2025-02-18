#include <Audio.h>
#include "MyDsp.h"

#define POT_PIN A0  // Broche du potentiomètre

MyDsp myDsp;
AudioOutputI2S audioOut;
AudioControlSGTL5000 audioShield;

AudioConnection patchCord1(myDsp, 0, audioOut, 0); // Canal gauche
AudioConnection patchCord2(myDsp, 1, audioOut, 1); // Canal droit

void setup() {
    AudioMemory(10);
    audioShield.enable();
    audioShield.volume(0.5);
}

void loop() {
    myDsp.setFreq(random(50,1000));
    int potValue = analogRead(POT_PIN); // Lecture du potentiomètre (0 à 1023)
    float pan = potValue / 1023.0; // Normalisation entre 0.0 et 1.0
    myDsp.setPan(pan); // Mise à jour du panning

    delay(5);
}
