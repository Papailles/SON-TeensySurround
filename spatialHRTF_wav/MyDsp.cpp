#include "MyDsp.h"
#include <Arduino.h>
#include <Audio.h>
#include <math.h>

#define MULT_16 32767

MyDsp::MyDsp() : AudioStream(1, inputQueueArray), currentAngle(0) {
}

void MyDsp::begin() {
    // Initialiser le moteur HRTF (le taux d'échantillonnage et la taille du bloc sont définis par la Teensy Audio Library)
    hrtfEngine.init(AUDIO_SAMPLE_RATE_EXACT, AUDIO_BLOCK_SAMPLES);
    
    // Charger le fichier binaire contenant les HRIR depuis la carte SD
    if (!hrtfEngine.loadFromBin("/hrtf_elev0.bin")) {
        Serial.println("Echec du loadFromBin => fallback manuel");
    } else {
        Serial.println("OK => HRIR chargé depuis bin!");
    }
}

void MyDsp::setAngle(int newAngle) {
    // Normaliser l'angle dans [0,359]
    if (newAngle < 0) {
        newAngle = (newAngle % 360 + 360) % 360;
    } else {
        newAngle = newAngle % 360;
    }
    currentAngle = newAngle;
}

int MyDsp::getAngle() const {
    return currentAngle;
}

void MyDsp::update() {
    audio_block_t* inBlock = receiveReadOnly(0);
    if (!inBlock) {
        return;
    }

    /* DEBUG : Afficher quelques échantillons bruts du inBlock pour vérifier les données reçues
    Serial.print("Raw inBlock samples: ");
    for (int i = 0; i < 10; i++) {
        Serial.print(inBlock->data[i]);
        Serial.print(" ");
    }
    Serial.println();
    */

    // Allouer les blocs de sortie pour chaque canal stéréo
    audio_block_t* outBlock[AUDIO_OUTPUTS];
    for (int c = 0; c < AUDIO_OUTPUTS; c++) {
        outBlock[c] = allocate();
        if (!outBlock[c]) {
            release(inBlock);
            return;
        }
    }

    // Conversion stéréo -> mono : on mixe les deux canaux en moyennant
    float inMono[AUDIO_BLOCK_SAMPLES];
    float maxIn = 0.0f;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float left  = inBlock->data[i * 2]     / 32768.0f;
        float right = inBlock->data[i * 2 + 1] / 32768.0f;
        inMono[i] = (left + right) * 0.5f;
        if (fabs(inMono[i]) > maxIn) {
            maxIn = fabs(inMono[i]);
        }
    }
    release(inBlock);

    // Sélection du HRIR en fonction de l'angle courant
    SelectedHrir sel = hrtfEngine.getHrir(currentAngle);

    // Calculer quelques indicateurs du HRIR (pour le canal gauche)
    float hrirMax = 0.0f;
    float hrirL1 = 0.0f;
    for (int i = 0; i < sel.length; i++) {
        float absVal = fabs(sel.left[i]);
        if (absVal > hrirMax) {
            hrirMax = absVal;
        }
        hrirL1 += absVal;
    }

    // Choisir un gain (à ajuster selon vos mesures)
    float gain = 1.0f;

    // Appel de la convolution avec overlap-add
    hrtfEngine.processBlock(inMono, outFloatLeft, outFloatRight, sel, gain);

    // Calculer le niveau maximum des sorties
    float maxOutL = 0.0f, maxOutR = 0.0f;
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        if (fabs(outFloatLeft[i]) > maxOutL) {
            maxOutL = fabs(outFloatLeft[i]);
        }
        if (fabs(outFloatRight[i]) > maxOutR) {
            maxOutR = fabs(outFloatRight[i]);
        }
        // Conversion en int16_t pour la sortie
        outBlock[0]->data[i] = (int16_t)(outFloatLeft[i] * MULT_16);
        outBlock[1]->data[i] = (int16_t)(outFloatRight[i] * MULT_16);
    }

    // Transmettre les blocs de sortie
    transmit(outBlock[0], 0);
    transmit(outBlock[1], 1);
    release(outBlock[0]);
    release(outBlock[1]);

    // Exemple de mise à jour automatique de l'angle (à désactiver si contrôle externe)
    setAngle(currentAngle + 1);

    // Impressions de débogage toutes les secondes
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint > 1000) {
        lastPrint = millis();
        Serial.print("Angle: ");
        Serial.print(currentAngle);
        Serial.print(" | HRIR: len=");
        Serial.print(sel.length);
        Serial.print(", delayL=");
        Serial.print(sel.delayLeft);
        Serial.print(", delayR=");
        Serial.print(sel.delayRight);
        Serial.print(" | Max In: ");
        Serial.print(maxIn, 4);
        Serial.print(" | Max Out L: ");
        Serial.print(maxOutL, 4);
        Serial.print(", Out R: ");
        Serial.println(maxOutR, 4);

        Serial.print("HRIR max (canal gauche): ");
        Serial.println(hrirMax, 4);
        Serial.print("HRIR L1 norm (canal gauche): ");
        Serial.println(hrirL1, 4);
        Serial.print("Gain appliqué: ");
        Serial.println(gain, 4);

        // Afficher les 5 premiers échantillons du signal d'entrée
        Serial.print("inMono[0..4]: ");
        for (int i = 0; i < 5; i++) {
            Serial.print(inMono[i], 4);
            Serial.print(" ");
        }
        Serial.println();

        // Afficher les 5 premiers échantillons de la sortie (canal gauche)
        Serial.print("outLeft[0..4]: ");
        for (int i = 0; i < 5; i++) {
            Serial.print(outFloatLeft[i], 4);
            Serial.print(" ");
        }
        Serial.println();

        // Afficher quelques valeurs de l'overlap (canal gauche)
        const float* ol = hrtfEngine.getOverlapLeft();
        int olSize = hrtfEngine.getOverlapSize();
        Serial.print("OverlapLeft[0..4]: ");
        for (int i = 0; i < 5 && i < olSize; i++) {
            Serial.print(ol[i], 4);
            Serial.print(" ");
        }
        Serial.println();
    }
}