#include "ProjectHrtfEngine.h"
#include <string.h>
#include <math.h>
#include <SD.h>
#include <SPI.h>

ProjectHrtfEngine::ProjectHrtfEngine()
: hrirCount(0), sampleRate(44100), blockSize(128), overlapSize(0)
{
    for (int i = 0; i < MAX_HRIR_SLOTS; i++) {
        hrirSlots[i].azimuth = 0;
        hrirSlots[i].data.delayLeft  = 0;
        hrirSlots[i].data.delayRight = 0;
        hrirSlots[i].data.length     = 0;
        memset(hrirSlots[i].data.left,  0, sizeof(hrirSlots[i].data.left));
        memset(hrirSlots[i].data.right, 0, sizeof(hrirSlots[i].data.right));
    }
    // Initialiser les buffers d'overlap à 0
    memset(overlapLeft, 0, sizeof(overlapLeft));
    memset(overlapRight, 0, sizeof(overlapRight));
}

void ProjectHrtfEngine::init(int sRate, int bSize) {
    sampleRate = sRate;
    blockSize  = bSize;
    hrirCount  = 0;
    overlapSize = 0;
    memset(overlapLeft, 0, sizeof(overlapLeft));
    memset(overlapRight, 0, sizeof(overlapRight));
}

void ProjectHrtfEngine::addHrir(int azimuthDeg,
                                const float* left, const float* right,
                                unsigned delayLeft, unsigned delayRight,
                                size_t length) {
    if (hrirCount >= MAX_HRIR_SLOTS) {
        return;
    }
    hrirSlots[hrirCount].azimuth = azimuthDeg;
    hrirSlots[hrirCount].data.delayLeft  = delayLeft;
    hrirSlots[hrirCount].data.delayRight = delayRight;
    hrirSlots[hrirCount].data.length     = (length > MAX_HRIR_LENGTH) ? MAX_HRIR_LENGTH : length;
    for (size_t i = 0; i < hrirSlots[hrirCount].data.length; i++) {
        hrirSlots[hrirCount].data.left[i]  = left[i];
        hrirSlots[hrirCount].data.right[i] = right[i];
    }
    for (size_t i = hrirSlots[hrirCount].data.length; i < MAX_HRIR_LENGTH; i++) {
        hrirSlots[hrirCount].data.left[i]  = 0.0f;
        hrirSlots[hrirCount].data.right[i] = 0.0f;
    }
    hrirCount++;
}

bool ProjectHrtfEngine::loadFromBin(const String &filename) {
    File f = SD.open(filename.c_str(), FILE_READ);
    if (!f) {
        Serial.print("Impossible d'ouvrir le fichier ");
        Serial.println(filename);
        return false;
    }

    char magic[4];
    if (f.read(magic, 4) < 4) {
        Serial.println("Lecture magic échouée");
        f.close();
        return false;
    }
    if (strncmp(magic, "HRIR", 4) != 0) {
        Serial.println("Fichier bin invalide: magic != 'HRIR'");
        f.close();
        return false;
    }

    uint32_t fileSampleRate = 0, binHrirLen = 0, M = 0;
    auto readU32 = [&](uint32_t &val) -> bool {
        byte tmp[4];
        if (f.read(tmp, 4) < 4) {
            return false;
        }
        val = (uint32_t)(tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24));
        return true;
    };
    if (!readU32(fileSampleRate) || !readU32(binHrirLen) || !readU32(M)) {
        Serial.println("Erreur de lecture des entiers dans le fichier bin");
        f.close();
        return false;
    }

    sampleRate = fileSampleRate;
    hrirCount = 0;
    for (uint32_t m = 0; m < M; m++) {
        if (hrirCount >= MAX_HRIR_SLOTS) break;

        // Lecture des angles et distances (on n'utilise ici que l'azimuth pour la sélection,
        // mais on stocke la distance pour l'atténuation)
        float az = 0.0f, el = 0.0f, dist = 0.0f;
        auto readFloat = [&]() -> float {
            byte tmp[4];
            if (f.read(tmp, 4) < 4) {
                return 0.0f;
            }
            union {
                uint32_t u;
                float f;
            } conv;
            conv.u = (uint32_t)(tmp[0] | (tmp[1] << 8) | (tmp[2] << 16) | (tmp[3] << 24));
            return conv.f;
        };
        az   = readFloat();
        el   = readFloat();
        dist = readFloat();

        int maxLen = (binHrirLen > MAX_HRIR_LENGTH) ? MAX_HRIR_LENGTH : binHrirLen;
        float leftBuf[MAX_HRIR_LENGTH];
        float rightBuf[MAX_HRIR_LENGTH];

        for (int i = 0; i < (int)binHrirLen; i++) {
            float val = readFloat();
            if (i < maxLen) {
                leftBuf[i] = val;
            }
        }
        for (int i = 0; i < (int)binHrirLen; i++) {
            float val = readFloat();
            if (i < maxLen) {
                rightBuf[i] = val;
            }
        }

        //Normalisation : calculer la valeur maximale absolue et diviser chaque échantillon par ce maximum
        float maxVal = 0.0f;
        for (int i = 0; i < maxLen; i++) {
            float absLeft = fabs(leftBuf[i]);
            float absRight = fabs(rightBuf[i]);
            if (absLeft > maxVal) maxVal = absLeft;
            if (absRight > maxVal) maxVal = absRight;
        }
        if (maxVal > 0.0f) {
            float normFactor = 1.0f / maxVal;
            for (int i = 0; i < maxLen; i++) {
                leftBuf[i]  *= normFactor;
                rightBuf[i] *= normFactor;
            }
        }
        

        // Stocker le HRIR normalisé dans le tableau des HRIR
        hrirSlots[hrirCount].azimuth = (int)roundf(az);
        hrirSlots[hrirCount].distance = dist;  // Stocker la distance lue
        hrirSlots[hrirCount].data.delayLeft  = 0;
        hrirSlots[hrirCount].data.delayRight = 0;
        hrirSlots[hrirCount].data.length     = maxLen;
        for (int i = 0; i < maxLen; i++) {
            hrirSlots[hrirCount].data.left[i]  = leftBuf[i];
            hrirSlots[hrirCount].data.right[i] = rightBuf[i];
        }
        for (int i = maxLen; i < MAX_HRIR_LENGTH; i++) {
            hrirSlots[hrirCount].data.left[i]  = 0.0f;
            hrirSlots[hrirCount].data.right[i] = 0.0f;
        }
        hrirCount++;
    }

    f.close();
    Serial.print("loadFromBin OK, hrirCount=");
    Serial.println(hrirCount);
    return true;
}

SelectedHrir ProjectHrtfEngine::getHrir(int azimuthDeg) {
    SelectedHrir sel;
    sel.left = nullptr;
    sel.right= nullptr;
    sel.delayLeft = 0;
    sel.delayRight = 0;
    sel.length = 0;
    sel.distance = 0.0f;  // si besoin d'utiliser la distance ailleurs

    if (hrirCount == 0) {
        return sel;
    }

    // Recherche du HRIR dont l'azimuth est le plus proche, en tenant compte de la circularité
    int bestIndex = 0;
    int bestDiff = 360;
    for (int i = 0; i < hrirCount; i++) {
        int diff = abs(azimuthDeg - hrirSlots[i].azimuth);
        if (diff > 180) {
            diff = 360 - diff;
        }
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIndex = i;
        }
    }
    
    // Récupérer les données du HRIR sélectionné
    sel.left  = hrirSlots[bestIndex].data.left;
    sel.right = hrirSlots[bestIndex].data.right;
    sel.length = hrirSlots[bestIndex].data.length;
    sel.distance = hrirSlots[bestIndex].distance; // si vous utilisez la distance plus tard

    // Si les délais stockés sont zéro, on calcule l'ITD approximatif basé sur l'azimut
    if (hrirSlots[bestIndex].data.delayLeft == 0 && hrirSlots[bestIndex].data.delayRight == 0) {
        // Convertir l'azimut en angle entre -180 et 180
        float effectiveAz = (azimuthDeg > 180) ? azimuthDeg - 360 : (float)azimuthDeg;
        float rad = effectiveAz * 3.14159265f / 180.0f;
        
        // Paramètres hypothétiques : rayon de la tête et vitesse du son
        float headRadius = 0.15f;      // en mètres (15 cm)
        float speedOfSound = 343.0f;       // en m/s
        // Calcul de l'ITD (en secondes) : ITD = (headRadius / speedOfSound) * sin(angle)
        float itd = headRadius / speedOfSound * sin(rad);
        // Convertir l'ITD en nombre d'échantillons
        unsigned delaySamples = (unsigned)round(fabs(itd) * sampleRate);
        
        // Appliquer le délai : si l'ITD est négatif, on retarde le canal gauche, sinon le canal droit
        if (itd < 0) {
            sel.delayLeft = delaySamples;
            sel.delayRight = 0;
        } else {
            sel.delayLeft = 0;
            sel.delayRight = delaySamples;
        }
    } else {
        sel.delayLeft  = hrirSlots[bestIndex].data.delayLeft;
        sel.delayRight = hrirSlots[bestIndex].data.delayRight;
    }
    
    return sel;
}

void ProjectHrtfEngine::processBlock(const float* in, float* outLeft, float* outRight,
                                     const SelectedHrir& selHrir, float gain) {
    // Longueur de la HRIR (nombre de taps)
    const int L = selHrir.length;
    // Taille étendue du buffer = blockSize + L - 1
    const int extSize = blockSize + L - 1;
    
    // Créer des buffers temporaires pour la convolution
    float tempL[extSize];
    float tempR[extSize];
    for (int i = 0; i < extSize; i++) {
         tempL[i] = 0.0f;
         tempR[i] = 0.0f;
    }
    
    // Ajouter l'overlap provenant du bloc précédent
    for (int i = 0; i < overlapSize; i++) {
         tempL[i] += overlapLeft[i];
         tempR[i] += overlapRight[i];
    }
    
    // Convolution naïve sur le bloc courant
    for (int n = 0; n < blockSize; n++) {
         float sample = in[n];
         for (int k = 0; k < L; k++) {
              // On ajoute contribution de in[n] * hrir[k] à la position n+k
              tempL[n + k] += sample * selHrir.left[k];
              tempR[n + k] += sample * selHrir.right[k];
         }
    }
    
    // Calcul du facteur d'atténuation basé sur la distance (loi inverse du carré)
    // Si la distance est inférieure ou égale à 1, on ne modifie pas.
    float distanceFactor = 1.0f;
    if (selHrir.distance > 1.0f) {
         distanceFactor = 1.0f / (selHrir.distance * selHrir.distance);
    }
    
    // Appliquer le gain global et le facteur de distance, et copier les blockSize premiers échantillons
    for (int n = 0; n < blockSize; n++) {
         outLeft[n]  = tempL[n] * gain * distanceFactor;
         outRight[n] = tempR[n] * gain * distanceFactor;
    }
    
    // Mise à jour de l'overlap-add : conserver la "queue" (L-1 échantillons) pour le prochain bloc
    const int newOverlapSize = L - 1;
    for (int n = 0; n < newOverlapSize; n++) {
         overlapLeft[n]  = tempL[blockSize + n];
         overlapRight[n] = tempR[blockSize + n];
    }
    overlapSize = newOverlapSize;
}




