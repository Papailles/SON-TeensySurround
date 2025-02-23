#include <Arduino.h>
#include <Audio.h>
#include "MyDsp.h"
#include <SPI.h>
#include <SD.h>

// Déclaration des objets audio
AudioPlaySdWav playWav1;         // Lecteur de fichiers WAV sur SD
AudioMixer4 mixer;               // Mixeur pour combiner les deux canaux en mono
AudioOutputI2S audioOutput;       // Sortie audio I2S (utilisée avec l'Audio Shield)
MyDsp myDsp;                     // Notre classe de traitement HRTF
AudioControlSGTL5000 audioShield;

// Connexions :
// On connecte les deux canaux de playWav1 au mixeur
AudioConnection patchCord1(playWav1, 0, mixer, 0);  // Canal gauche de playWav1 vers entrée 0 du mixeur
AudioConnection patchCord2(playWav1, 1, mixer, 1);  // Canal droit de playWav1 vers entrée 1 du mixeur

// On règle le gain des deux entrées à 0.5 pour obtenir la moyenne
// (les entrées 2 et 3 restent inutilisées, leur gain peut être laissé à 0)
  
// On connecte la sortie du mixeur (canal 0) à l'entrée mono de MyDsp
AudioConnection patchCord3(mixer, 0, myDsp, 0);

// Puis, on connecte la sortie de MyDsp aux sorties gauche et droite de audioOutput
AudioConnection patchCord4(myDsp, 0, audioOutput, 0); // sortie gauche
AudioConnection patchCord5(myDsp, 1, audioOutput, 1); // sortie droite


void setup() {
  Serial.begin(115200);
  while (!Serial) { } // Attendre l'ouverture du moniteur série

  AudioMemory(12);

  // Initialisation du shield audio
  audioShield.enable();
  audioShield.volume(0.3);

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

  // Réglage des gains du mixeur pour faire la moyenne
  mixer.gain(0, 0.5);
  mixer.gain(1, 0.5);

  // Initialiser le DSP HRTF (chargement des HRIR, etc.)
  myDsp.begin();

  // Démarrer la lecture du fichier WAV "music.wav"
  // Vérifiez que le fichier "music.wav" est à la racine de la carte SD
  if (!playWav1.play("music.wav")) {
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
