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

        // Lecture des angles et distances (on n'utilise ici que l'azimuth)
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

        // Normalisation : calculer la valeur maximale absolue et diviser chaque échantillon par ce maximum
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
    sel.right = nullptr;
    sel.delayLeft = 0;
    sel.delayRight = 0;
    sel.length = 0;

    if (hrirCount == 0) {
        return sel;
    }

    // Recherche du HRIR dont l'azimuth est le plus proche,
    // en prenant en compte la circularité (différence minimale modulo 360)
    int bestIndex = 0;
    int bestDiff = 360; // différence maximale possible
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
    sel.left  = hrirSlots[bestIndex].data.left;
    sel.right = hrirSlots[bestIndex].data.right;
    sel.delayLeft  = hrirSlots[bestIndex].data.delayLeft;
    sel.delayRight = hrirSlots[bestIndex].data.delayRight;
    sel.length     = hrirSlots[bestIndex].data.length;
    return sel;
}

void ProjectHrtfEngine::processBlock(const float* in, float* outLeft, float* outRight,
                                     const SelectedHrir& selHrir, float gain) {
    // Longueur de l'impulsion HRIR
    int L = selHrir.length;
    // Taille du bloc étendu = blockSize + L - 1
    int extendedSize = blockSize + L - 1;
    // On suppose ici extendedSize <= 255 (avec blockSize = 128 et MAX_HRIR_LENGTH = 128)
    float tempLeft[255];
    float tempRight[255];
    memset(tempLeft, 0, extendedSize * sizeof(float));
    memset(tempRight, 0, extendedSize * sizeof(float));

    // Copier l'overlap de l'appel précédent dans le début du buffer temporaire
    for (int i = 0; i < overlapSize; i++) {
         tempLeft[i] += overlapLeft[i];
         tempRight[i] += overlapRight[i];
    }

    // Calculer le niveau moyen (moyenne absolue) du bloc d'entrée
    float avgIn = 0.0f;
    for (int n = 0; n < blockSize; n++) {
         avgIn += fabs(in[n]);
    }
    avgIn /= blockSize;

    // Gestion conditionnelle de l'overlap en fonction du niveau d'entrée
    const float silenceThreshold = 0.005f;  // Seuil à ajuster selon vos tests
    const float fadeFactor = 0.5f;          // Facteur de décroissance si le signal n'est pas complètement silencieux

    if (avgIn < silenceThreshold) {
         // En cas de silence, on réinitialise complètement l'overlap
         for (int i = 0; i < overlapSize; i++) {
             overlapLeft[i] = 0.0f;
             overlapRight[i] = 0.0f;
         }
    } else {
         // Sinon, on applique un fade-out sur l'overlap pour réduire progressivement l'énergie résiduelle
         for (int i = 0; i < overlapSize; i++) {
              overlapLeft[i] *= fadeFactor;
              overlapRight[i] *= fadeFactor;
         }
    }

    // Copier l'overlap (après gestion) dans tempLeft et tempRight
    // (Si la gestion précédente a modifié overlap, les valeurs copiées seront celles atténuées ou réinitialisées)
    for (int i = 0; i < overlapSize; i++) {
         tempLeft[i] = overlapLeft[i];
         tempRight[i] = overlapRight[i];
    }

    // Convolution sur le bloc courant
    for (int n = 0; n < blockSize; n++) {
         for (int k = 0; k < L; k++) {
             tempLeft[n + k] += in[n] * selHrir.left[k];
             tempRight[n + k] += in[n] * selHrir.right[k];
         }
    }

    // Appliquer le gain et copier les blockSize premiers échantillons dans la sortie
    for (int n = 0; n < blockSize; n++) {
         outLeft[n]  = tempLeft[n] * gain;
         outRight[n] = tempRight[n] * gain;
    }

    // Mise à jour de l'overlap : la partie excédentaire (la "queue" de la convolution)
    int newOverlapSize = L - 1;
    for (int n = 0; n < newOverlapSize; n++) {
         overlapLeft[n]  = tempLeft[blockSize + n];
         overlapRight[n] = tempRight[blockSize + n];
    }
    overlapSize = newOverlapSize;
}


