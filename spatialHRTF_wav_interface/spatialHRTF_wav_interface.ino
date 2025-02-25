#include <Arduino.h>
#include <Audio.h>
#include "MyDsp.h"
#include <SPI.h>
#include <SD.h>

#define MAX_FILES 50

// Tableaux et variables pour stocker la liste des fichiers WAV
String wavFiles[MAX_FILES];
int fileCount = 0;
int currentFileIndex = 0;
bool paused = false;  // Indique si la lecture est en pause

// Déclaration des objets audio
AudioPlaySdWav playWav1;         // Lecteur de fichiers WAV sur SD
AudioMixer4 mixer;               // Mixeur pour combiner les deux canaux en mono
AudioOutputI2S audioOutput;       // Sortie audio I2S (utilisée avec l'Audio Shield)
MyDsp myDsp;                     // Notre classe de traitement HRTF
AudioControlSGTL5000 audioShield;

// Connexions audio
AudioConnection patchCord1(playWav1, 0, mixer, 0);
AudioConnection patchCord2(playWav1, 1, mixer, 1);
AudioConnection patchCord3(mixer, 0, myDsp, 0);
AudioConnection patchCord4(myDsp, 0, audioOutput, 0);
AudioConnection patchCord5(myDsp, 1, audioOutput, 1);

// Variables pour le contrôle de l'angle via le port série
volatile bool manualMode = false;     // false = mode auto, true = mode manuel
String serialCommand = "";

// Fonction qui charge dans wavFiles[] tous les fichiers .wav trouvés dans la racine
void loadWavFileList() {
  fileCount = 0;
  File root = SD.open("/");
  while (true) {
    File entry = root.openNextFile();
    if (!entry) break; // Plus de fichiers
    if (!entry.isDirectory()) {
      String fname = entry.name();
      if (fname.endsWith(".wav") || fname.endsWith(".WAV")) {
        if (fileCount < MAX_FILES) {
          wavFiles[fileCount] = fname;
          fileCount++;
        }
      }
    }
    entry.close();
  }
  root.close();
}

// Fonction récursive pour lister les fichiers (pour affichage sur le moniteur série)
void listFiles(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    for (int i = 0; i < numTabs; i++) {
      Serial.print("\t");
    }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      listFiles(entry, numTabs + 1);
    } else {
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

// Fonction pour traiter les commandes série reçues
void processSerialCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  
  if (cmd.startsWith("MODE:")) {
    String mode = cmd.substring(5);
    mode.trim();
    if (mode.equalsIgnoreCase("MANUEL")) {
      manualMode = true;
      Serial.println("Mode Manuel activé");
    } else if (mode.equalsIgnoreCase("AUTO")) {
      manualMode = false;
      Serial.println("Mode Auto activé");
    } else {
      Serial.println("Mode inconnu");
    }
  }
  else if (cmd.startsWith("SET_ANGLE:")) {
    if (manualMode) {
      String angleStr = cmd.substring(10);  // "SET_ANGLE:" fait 10 caractères
      angleStr.trim();
      int angle = angleStr.toInt();
      myDsp.setAngle(angle);
      Serial.print("SET_ANGLE:");
      Serial.println(myDsp.getAngle());
    } else {
      Serial.println("Commande SET_ANGLE ignorée en mode Auto");
    }
  }
  else if (cmd.equalsIgnoreCase("GET_ANGLE")) {
    int currentAngle = myDsp.getAngle();
    Serial.print("GET_ANGLE:");
    Serial.println(currentAngle);
  }
  else if (cmd.equalsIgnoreCase("PREV")) {
    if (fileCount > 0) {
      currentFileIndex = (currentFileIndex - 1 + fileCount) % fileCount;
      if (!playWav1.play(wavFiles[currentFileIndex].c_str())) {
        Serial.print("Erreur: impossible de lire le fichier ");
        Serial.println(wavFiles[currentFileIndex]);
      } else {
        Serial.print("TRACK:");
        Serial.println(wavFiles[currentFileIndex]);
        paused = false;
      }
    }
  }
  else if (cmd.equalsIgnoreCase("NEXT")) {
    if (fileCount > 0) {
      currentFileIndex = (currentFileIndex + 1) % fileCount;
      if (!playWav1.play(wavFiles[currentFileIndex].c_str())) {
        Serial.print("Erreur: impossible de lire le fichier ");
        Serial.println(wavFiles[currentFileIndex]);
      } else {
        Serial.print("TRACK:");
        Serial.println(wavFiles[currentFileIndex]);
        paused = false;
      }
    }
  }
  else if (cmd.equalsIgnoreCase("PAUSE")) {
    if (!paused && playWav1.isPlaying()) {
      playWav1.stop();
      paused = true;
      Serial.print("TRACK:");
      Serial.print(wavFiles[currentFileIndex]);
      Serial.println(" PAUSED");
    }
  }
  else if (cmd.equalsIgnoreCase("PLAY")) {
    if (paused) {
      if (!playWav1.play(wavFiles[currentFileIndex].c_str())) {
        Serial.print("Erreur: impossible de relancer la lecture du fichier ");
        Serial.println(wavFiles[currentFileIndex]);
      } else {
        paused = false;
        Serial.print("TRACK:");
        Serial.println(wavFiles[currentFileIndex]);
      }
    }
  }
  else if (cmd.startsWith("VOLUME:")) {
    String volStr = cmd.substring(7);  // "VOLUME:" fait 7 caractères
    volStr.trim();
    int volPercent = volStr.toInt();
    if (volPercent < 0) volPercent = 0;
    if (volPercent > 100) volPercent = 100;
    float vol = volPercent / 100.0;
    audioShield.volume(vol);
    Serial.print("VOLUME:");
    Serial.println(volPercent);
}
  else {
    Serial.print("Commande inconnue: ");
    Serial.println(cmd);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { } // Attendre l'ouverture du port série

  Serial.println("Attente de la connexion de l'interface...");
  // Attente active jusqu'à réception de "CONNECT" depuis l'interface
  bool interfaceConnected = false;
  while (!interfaceConnected) {
    if (Serial.available() > 0) {
      String handshake = Serial.readStringUntil('\n');
      handshake.trim();
      if (handshake.equalsIgnoreCase("CONNECT")) {
        interfaceConnected = true;
        Serial.println("Interface connectée.");
      } else {
        Serial.print("Commande ignorée : ");
        Serial.println(handshake);
      }
    }
    delay(100);
  }

  // Initialisation de la chaîne audio
  AudioMemory(16);

  // Initialisation du shield audio
  audioShield.enable();
  audioShield.volume(0.4);
  Serial.print("Configuration audio - Sample Rate: ");
  Serial.println(AUDIO_SAMPLE_RATE_EXACT);

  // Initialisation de la carte SD
  if (!SD.begin()) {
    Serial.println("Erreur: impossible d'initialiser la carte SD !");
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  Serial.println("Carte SD initialisée avec succès.");

  // Affichage de la liste des fichiers sur le moniteur série
  File root = SD.open("/");
  listFiles(root, 0);
  root.close();

  // Charger la liste des fichiers WAV
  loadWavFileList();
  Serial.print("Nombre de fichiers WAV trouvés: ");
  Serial.println(fileCount);
  for (int i = 0; i < fileCount; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(wavFiles[i]);
  }

  // Réglage des gains du mixeur
  mixer.gain(0, 0.5);
  mixer.gain(1, 0.5);

  // Initialisation du DSP HRTF
  myDsp.begin();

  // Démarrer la lecture du premier fichier WAV s'il y en a
  if (fileCount > 0) {
    currentFileIndex = 0;
    if (!playWav1.play(wavFiles[currentFileIndex].c_str())) {
      Serial.print("Erreur: impossible de lire le fichier ");
      Serial.println(wavFiles[currentFileIndex]);
    } else {
      Serial.print("TRACK:");
      Serial.println(wavFiles[currentFileIndex]);
      paused = false;
    }
  } else {
    Serial.println("Aucun fichier WAV trouvé.");
  }
  delay(25);  // Pause brève pour l'en-tête WAV
}

void loop() {
  static unsigned long lastStatusTime = 0;
  static unsigned long lastProgressTime = 0;
  unsigned long currentTime = millis();

  // Traitement non bloquant des commandes série
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      processSerialCommand(serialCommand);
      serialCommand = "";
    } else {
      serialCommand += c;
    }
  }

  // Vérifier l'état de la lecture toutes les secondes (si non en pause)
  if (currentTime - lastStatusTime >= 1000) {
    if (!paused && !playWav1.isPlaying()) {
      Serial.println("Lecture terminée ou en pause. Passage au fichier suivant...");
      if (fileCount > 0) {
        currentFileIndex = (currentFileIndex + 1) % fileCount;
        if (!playWav1.play(wavFiles[currentFileIndex].c_str())) {
          Serial.print("Erreur: impossible de lire le fichier ");
          Serial.println(wavFiles[currentFileIndex]);
        } else {
          Serial.print("TRACK:");
          Serial.println(wavFiles[currentFileIndex]);
          paused = false;
        }
      }
    }
    lastStatusTime = currentTime;
  }

  // Envoyer la progression de la lecture toutes les 500 ms
  if (!paused && playWav1.isPlaying() && (currentTime - lastProgressTime >= 500)) {
    unsigned long pos = playWav1.positionMillis();
    unsigned long len = playWav1.lengthMillis();
    if (len > 0) {
      int progress = (int)((pos * 100UL) / len);
      Serial.print("PROGRESS:");
      Serial.println(progress);
    }
    lastProgressTime = currentTime;
  }
}
