#ifndef PROJECT_HRTF_ENGINE_H
#define PROJECT_HRTF_ENGINE_H

#include <Arduino.h>
#include <stddef.h>

// Longueur maximale d'une HRIR
static const int MAX_HRIR_LENGTH = 128;

struct HrirData {
    unsigned delayLeft;   // en échantillons
    unsigned delayRight;  // en échantillons
    float left[MAX_HRIR_LENGTH];
    float right[MAX_HRIR_LENGTH];
    size_t length;
};

struct SelectedHrir {
    const float* left;
    const float* right;
    unsigned delayLeft;
    unsigned delayRight;
    size_t length;
    float distance;  // Nouvelle donnée : distance en mètres (par exemple)
};

class ProjectHrtfEngine {
public:
    ProjectHrtfEngine();

    // Initialisation : sampleRate, blockSize (ex : 44100, 128)
    void init(int sRate, int bSize);
    void addHrir(int azimuthDeg,
                 const float* left, const float* right,
                 unsigned delayLeft, unsigned delayRight,
                 size_t length);
    bool loadFromBin(const String &filename);
    SelectedHrir getHrir(int azimuthDeg);

    // Convolution naïve avec overlap-add et gain
    void processBlock(const float* in, float* outLeft, float* outRight,
                      const SelectedHrir& selHrir, float gain = 1.0f);

    const float* getOverlapLeft() const { return overlapLeft; }
    int getOverlapSize() const { return overlapSize; }

private:
    static const int MAX_HRIR_SLOTS = 128;
    struct HrirSlot {
        int azimuth;
        float distance; // Nouvelle donnée pour stocker la distance
        HrirData data;
    };

    HrirSlot hrirSlots[MAX_HRIR_SLOTS];
    int hrirCount;
    int sampleRate;
    int blockSize;

    // Buffers pour overlap-add
    float overlapLeft[MAX_HRIR_LENGTH];
    float overlapRight[MAX_HRIR_LENGTH];
    int overlapSize;
};

#endif
