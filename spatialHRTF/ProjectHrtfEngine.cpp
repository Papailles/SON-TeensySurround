#include "ProjectHrtfEngine.h"
#include <string.h> // pour memset etc.
#include <math.h>   // pour abs()

ProjectHrtfEngine::ProjectHrtfEngine()
: hrirCount(0),
  sampleRate(44100),
  blockSize(128)
{
    // Init par défaut
    for(int i=0; i<MAX_HRIR_SLOTS; i++){
        hrirSlots[i].azimuth = 0;
        hrirSlots[i].data.delayLeft  = 0;
        hrirSlots[i].data.delayRight = 0;
        hrirSlots[i].data.length     = 0;
        memset(hrirSlots[i].data.left,  0, sizeof(hrirSlots[i].data.left));
        memset(hrirSlots[i].data.right, 0, sizeof(hrirSlots[i].data.right));
    }
}

void ProjectHrtfEngine::init(int sRate, int bSize)
{
    sampleRate = sRate;
    blockSize  = bSize;
    hrirCount  = 0;
}

void ProjectHrtfEngine::addHrir(int azimuthDeg,
                                const float* left, const float* right,
                                unsigned delayLeft, unsigned delayRight,
                                size_t length)
{
    if(hrirCount >= MAX_HRIR_SLOTS) {
        // Plus de place
        return;
    }
    hrirSlots[hrirCount].azimuth = azimuthDeg;
    hrirSlots[hrirCount].data.delayLeft  = delayLeft;
    hrirSlots[hrirCount].data.delayRight = delayRight;
    hrirSlots[hrirCount].data.length     = length;

    for(size_t i=0; i<length && i<MAX_HRIR_LENGTH; i++){
        hrirSlots[hrirCount].data.left[i]  = left[i];
        hrirSlots[hrirCount].data.right[i] = right[i];
    }
    // zéro pour le reste
    for(size_t i=length; i<MAX_HRIR_LENGTH; i++){
        hrirSlots[hrirCount].data.left[i]  = 0.0f;
        hrirSlots[hrirCount].data.right[i] = 0.0f;
    }

    hrirCount++;
}

SelectedHrir ProjectHrtfEngine::getHrir(int azimuthDeg)
{
    SelectedHrir sel;
    sel.left = nullptr;
    sel.right= nullptr;
    sel.delayLeft= 0;
    sel.delayRight=0;
    sel.length=0;

    if(hrirCount == 0) {
        // Aucune HRIR
        return sel;
    }

    // On recherche la HRIR la plus proche en azimuth
    int bestIndex = 0;
    int bestDist  = 999999;
    for(int i=0; i<hrirCount; i++){
        int dist = abs(azimuthDeg - hrirSlots[i].azimuth);
        if(dist < bestDist){
            bestDist = dist;
            bestIndex= i;
        }
    }

    sel.left  = hrirSlots[bestIndex].data.left;
    sel.right = hrirSlots[bestIndex].data.right;
    sel.delayLeft  = hrirSlots[bestIndex].data.delayLeft;
    sel.delayRight = hrirSlots[bestIndex].data.delayRight;
    sel.length = hrirSlots[bestIndex].data.length;
    return sel;
}

// Convolution naïve en O(n²)
void ProjectHrtfEngine::processBlock(const float* in, float* outLeft, float* outRight,
                                     const SelectedHrir& selHrir)
{
    // On met outLeft/outRight à 0
    memset(outLeft,  0, blockSize*sizeof(float));
    memset(outRight, 0, blockSize*sizeof(float));

    if(!selHrir.left || !selHrir.right || selHrir.length == 0) {
        // HRIR invalide
        return;
    }

    for(int n=0; n<blockSize; n++){
        float x = in[n];
        // y[n] += sum_{k=0..n} in[n-k]*h[k]
        for(int k=0; k<=n && k<(int)selHrir.length; k++){
            outLeft[n]  += x * selHrir.left[n-k];
            outRight[n] += x * selHrir.right[n-k];
        }
    }

    // On ignore l'ITD (delayLeft/delayRight) dans cet exemple
    // Il faudrait un buffer circulaire pour appliquer un décalage
}
