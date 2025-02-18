#include <Audio.h>

// Définition des objets audio
AudioInputI2S        audioInput;     // Entrée audio I2S (depuis le shield audio)
AudioMixer4          mixerL, mixerR; // Mixeurs pour répartir le signal entre gauche et droite
AudioOutputI2S       audioOutput;    // Sortie audio I2S (vers casque ou haut-parleurs)
AudioConnection      patchCord1(audioInput, 0, mixerL, 0);
AudioConnection      patchCord2(audioInput, 0, mixerR, 0);
AudioConnection      patchCord3(mixerL, 0, audioOutput, 0); // Gauche
AudioConnection      patchCord4(mixerR, 0, audioOutput, 1); // Droite
AudioControlSGTL5000 audioShield;   // Contrôle du chip audio

const int potPin = A0;  // Potentiomètre sur A0

void setup() {
    AudioMemory(12);
    audioShield.enable();
    audioShield.volume(0.5);  // Volume global
    
    // Initialisation du mixeur
    mixerL.gain(0, 0.5);  // Gain initial : 50% gauche
    mixerR.gain(0, 0.5);  // Gain initial : 50% droite
}

void loop() {
  int potValue = analogRead(potPin);
  float pan = potValue / 1023.0;  // Valeur comprise entre 0.0 et 1.0
  myDsp.setPosition(pan, 0);      // Met à jour le panoramique
  myDsp.setFreq(1000)
  delay(300);
}
