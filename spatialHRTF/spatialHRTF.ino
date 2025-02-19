#include <Audio.h>
#include "MyDsp.h"

// Sortie audio
AudioOutputI2S audioOutput;
AudioInputI2S  audioInput;
// Notre DSP
MyDsp myDsp;

// On connecte myDsp -> audioOutput
AudioConnection      c1(audioInput, 0, myDsp, 0);
AudioConnection      c2(myDsp, 0, audioOutput, 0);
AudioConnection      c3(myDsp, 1, audioOutput, 1);
AudioControlSGTL5000 audioShield;

void setup() {
  AudioMemory(12); 
  audioShield.enable();
  audioShield.volume(1);

  // Initialiser MyDsp
  myDsp.begin();
}

void loop() {
  // rien de spécial, l'audio tourne en IRQ 
  myDsp.setFreq(1000);
  delay(100);
}
