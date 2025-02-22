#include <Arduino.h>
#include <Audio.h>
#include "MyDsp.h"
#include <SPI.h>
#include <SD.h>

AudioPlaySdWav playWav1;         // Lecteur de fichiers WAV sur SD
AudioOutputI2S audioOutput;       // Sortie audio I2S (utilisée avec l'Audio Shield)
MyDsp myDsp;                     // Notre classe de traitement HRTF
AudioConnection patchCord1(playWav1, 0, myDsp, 0);  // Connecte la sortie du WAV à l'entrée de MyDsp
AudioConnection patchCord2(myDsp, 0, audioOutput, 0); // Sortie gauche de MyDsp
AudioConnection patchCord3(myDsp, 1, audioOutput, 1); // Sortie droite de MyDsp
AudioControlSGTL5000 audioShield;

void setup() {
  Serial.begin(115200);
  while (!Serial) { } // Attendre l'ouverture du moniteur série

  AudioMemory(12);

  // Initialisation du shield audio
  audioShield.enable();
  audioShield.volume(0.4);

  // Afficher le taux d'échantillonnage (optionnel)
  Serial.print("Configuration audio - Sample Rate: ");
  Serial.println(AUDIO_SAMPLE_RATE_EXACT);

  // Initialiser la carte SD
  if (!SD.begin()) {
    Serial.println("Erreur: impossible d'initialiser la carte SD !");
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  Serial.println("Carte SD initialisée avec succès.");

  // Initialiser le DSP HRTF (chargement des HRIR, etc.)
  myDsp.begin();

  // Démarrer la lecture du fichier WAV "music.wav"
  // Vérifiez que le fichier "music.wav" est à la racine de la carte SD
  if (!playWav1.play("whitenoise.wav")) {
    Serial.println("Erreur: impossible de lire le fichier music.wav !");
  } else {
    Serial.println("Lecture du fichier music.wav en cours...");
  }
  delay(25);  // Pause brève pour laisser le temps de lire l'en-tête WAV
}

void loop() {
  // Affichage de l'état de lecture toutes les secondes
  if (playWav1.isPlaying()) {
    //Serial.println("Playing...");
  } else {
    Serial.println("Lecture terminée ou en pause.");
  }

  // Exemple de contrôle dynamique de l'angle (incrémentation)
  // Décommentez les deux lignes suivantes pour faire varier l'angle automatiquement
  //int angle = myDsp.getAngle();
  //myDsp.setAngle((angle + 1) % 360);

  delay(1000);
}
