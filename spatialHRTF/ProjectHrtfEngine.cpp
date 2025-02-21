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


/* -----------------------------------------------------------
   NOUVEAU : Chargement du .bin
   Format attendu :
    4 octets : "HRIR"
    4 octets : sampleRate (uint32_t)
    4 octets : hrirLen (uint32_t)
    4 octets : M (uint32_t)
    -- pour chaque mesure M :
       float az, el, dist (on n'utilise que az pour l'instant)
       float left[hrirLen], float right[hrirLen]
   ----------------------------------------------------------- */
bool ProjectHrtfEngine::loadFromBin(const String &filename)
{
    // Ouvrir le fichier sur la SD (ex: "hrtf_nh2.bin")
    File f = SD.open(filename, FILE_READ);
    if(!f) {
        Serial.print("Impossible d'ouvrir le fichier ");
        Serial.println(filename);
        return false;
    }

    char magic[4];
    if(f.read(magic, 4) < 4) {
        Serial.println("Lecture magic echouee");
        f.close();
        return false;
    }

    if(strncmp(magic, "HRIR", 4) != 0) {
        Serial.println("Fichier bin invalide: magic != 'HRIR'");
        f.close();
        return false;
    }

    uint32_t fileSampleRate=0, binHrirLen=0, M=0;
    // lecture en little-endian
    // Sur Teensy/Arduino, on peut faire:
    auto readU32 = [&](uint32_t &val){
      byte tmp[4];
      f.read(tmp, 4);
      val = (uint32_t)(tmp[0] | (tmp[1]<<8) | (tmp[2]<<16) | (tmp[3]<<24));
    };
    readU32(fileSampleRate);
    readU32(binHrirLen);
    readU32(M);

    Serial.print("loadFromBin: sampleRate=");
    Serial.print(fileSampleRate);
    Serial.print(", hrirLen=");
    Serial.print(binHrirLen);
    Serial.print(", M=");
    Serial.println(M);

    // On met sampleRate local
    sampleRate = fileSampleRate;

    hrirCount=0;
    for(uint32_t m=0; m<M; m++){
        if(hrirCount >= MAX_HRIR_SLOTS) break;

        // Lire az, el, dist => 3 floats
        float az, el, dist;
        auto readFloat = [&](){
            byte tmp[4];
            f.read(tmp,4);
            // reinterpret => float 32 bits LE
            union {
              uint32_t u;
              float ff;
            } conv;
            conv.u = (uint32_t)(tmp[0] | (tmp[1]<<8) | (tmp[2]<<16) | (tmp[3]<<24));
            return conv.ff;
        };
        az   = readFloat();
        el   = readFloat();
        dist = readFloat();

        // Lecture des hrirLen échantillons gauche
        // On peut en stocker max 128 dans data.left
        int maxLen = (binHrirLen > MAX_HRIR_LENGTH) ? MAX_HRIR_LENGTH : binHrirLen;
        float leftBuf[MAX_HRIR_LENGTH];
        float rightBuf[MAX_HRIR_LENGTH];

        for(int i=0; i<(int)binHrirLen; i++){
            if(i<maxLen) {
                // lire un float
                leftBuf[i] = readFloat();
            } else {
                // si binHrirLen > 128, on lit mais on jette
                (void)readFloat();
            }
        }
        for(int i=0; i<(int)binHrirLen; i++){
            if(i<maxLen) {
                rightBuf[i] = readFloat();
            } else {
                (void)readFloat();
            }
        }

        // on stocke
        hrirSlots[hrirCount].azimuth = (int)roundf(az); // on force un int
        hrirSlots[hrirCount].data.delayLeft = 0; 
        hrirSlots[hrirCount].data.delayRight= 0;
        hrirSlots[hrirCount].data.length = maxLen;

        for(int i=0; i<maxLen; i++){
            hrirSlots[hrirCount].data.left[i]  = leftBuf[i];
            hrirSlots[hrirCount].data.right[i] = rightBuf[i];
        }
        for(int i=maxLen; i<MAX_HRIR_LENGTH; i++){
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
        Serial.println("KOOOOO");
        return;
    }

    // Convolution naïve : chaque échantillon du bloc
    for(int n=0; n<blockSize; n++){
        float x = in[n];
        // y[n] += sum_{k=0..n} in[n-k]*h[k]
        for(int k=0; k<=n && k<(int)selHrir.length; k++){
            outLeft[n]  += x * selHrir.left[n-k];
            outRight[n] += x * selHrir.right[n-k];
        }
    }

    // Delais ITD si besoin
    unsigned dL = selHrir.delayLeft;
    unsigned dR = selHrir.delayRight;
    if(dL > 0 || dR > 0){
        float tempL[blockSize], tempR[blockSize];
        memcpy(tempL, outLeft,  blockSize*sizeof(float));
        memcpy(tempR, outRight, blockSize*sizeof(float));
        for(int i=0; i<blockSize; i++){
            if(i< (int)dL) outLeft[i]=0.0f; 
            else outLeft[i]=tempL[i-dL];
        }
        for(int i=0; i<blockSize; i++){
            if(i< (int)dR) outRight[i]=0.0f; 
            else outRight[i]=tempR[i-dR];
        }
    }
}
