#include <Audio.h>
#include "MyDsp.h"
#include "ProjectHrtfEngine.h"

AudioControlSGTL5000 audioShield;
AudioOutputI2S audioOutput;
MyDsp myDsp;

// Patch cords
AudioConnection pcLeft(myDsp, 0, audioOutput, 0);
AudioConnection pcRight(myDsp,1, audioOutput, 1);

void setup() {
  AudioMemory(12);
  audioShield.enable();
  audioShield.volume(0.8);

  myDsp.begin();
  // On peut régler la fréquence
  myDsp.setFreq(220.0f);
}

void loop() {
  // On pourrait varier la fréquence
  //myDsp.setFreq(random(200,3000));
  myDsp.setFreq(800);
  delay(1000);
}
