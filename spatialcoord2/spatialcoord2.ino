#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioOutputI2S audioOut;
AudioControlSGTL5000 audioShield;

AudioConnection patchCord1(myDsp, 0, audioOut, 0);
AudioConnection patchCord2(myDsp, 1, audioOut, 1);

float mtof(float note){
  return pow(2.0,(note-69.0)/12.0)*440.0;
}


int tune[] = {60, 62, 57, 60, 62, 57, 62, 59, 62, 57, 59, 62, 55, 62};
int cnt = 0;


void setup() {
    AudioMemory(2);
    audioShield.enable();
    audioShield.volume(0.5);
}

void loop() {
    // Changement de position pour tester
    myDsp.setPosition(1, 0);
    myDsp.setFreq(mtof(tune[cnt]));       
    cnt = (cnt+1)%5;
    delay(1000);
}
