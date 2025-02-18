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
    audioShield.volume(0.3);

    // Définition d’une position fixe pour tester la spatialisation
    myDsp.setPosition(0.5, -0.3); // Son à droite et légèrement éloigné
}

/*void loop() {
  myDsp.setFreq(mtof(tune[cnt]));
  myDsp.setPosition(0.5, 0.4); // Son à droite et légèrement éloigné
  cnt = (cnt+1)%5;
  delay(500);
}*/

void loop() {
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n'); // Lecture de la ligne complète
        float x, y;
        sscanf(command.c_str(), "%f %f", &x, &y); // Extraction des valeurs
        myDsp.setFreq(mtof(tune[cnt]));       
        cnt = (cnt+1)%5;
        myDsp.setPosition(x, y); // Mise à jour de la position
        Serial.print("Reçu: x = ");
        Serial.print(x);
        Serial.print(", y = ");
        Serial.println(y);
        delay(500);
    }
}
