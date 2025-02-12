#include <Audio.h>
#include "MyDsp.h"

MyDsp myDsp;
AudioOutputI2S out; //Sortie audio qui produit de l'I2S - protocole de communication entre Teenzy et puce audio
AudioControlSGTL5000 audioShield; //Interface qui permet de controller le codec audio (carte son) - vol in / out, sampling, buffer...
AudioConnection patchCord0(myDsp,0,out,0); //Câblage virtuel entre la première sortie de myDsp et la première entrée de output
AudioConnection patchCord1(myDsp,0,out,1); //myDsp a une seule sortie, donc deux câbles pour gauche / droite 

float mtof(float note){
  return pow(2.0,(note-69.0)/12)*440.0;
}

int tune[] = {62,78,65,67,69};
int cnt = 0;

void setup() {
  AudioMemory(2); //Le nb doit correspondre au moins au nombre de câbles instanciés
  audioShield.enable(); //Méthode pour activer la carte audio
  audioShield.volume(0.5); //Volume hardware de sortie de la prise jack - entre 0 et 1
}

void loop() {
  //myDsp.setFreq(random(50,1000));
  myDsp.setFreq(mtof(tune[cnt]));
  cnt = (cnt+1)%5;
  delay(500);
}
