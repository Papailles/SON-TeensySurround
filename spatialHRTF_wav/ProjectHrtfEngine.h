#ifndef PROJECT_HRTF_ENGINE_H
#define PROJECT_HRTF_ENGINE_H

#include <Arduino.h>
#include <stddef.h>

static const int MAX_HRIR_LENGTH = 128;

struct HrirData {
    unsigned delayLeft;
    unsigned delayRight;
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
};

class ProjectHrtfEngine {
public:
    ProjectHrtfEngine();

    void init(int sRate, int bSize);
    void addHrir(int azimuthDeg,
                 const float* left, const float* right,
                 unsigned delayLeft, unsigned delayRight,
                 size_t length);
    bool loadFromBin(const String &filename);
    SelectedHrir getHrir(int azimuthDeg);

    // Nouvelle version avec overlap-add
    // Le facteur de gain est ajouté (par défaut 1.0, à ajuster selon vos mesures)
    void processBlock(const float* in, float* outLeft, float* outRight,
                      const SelectedHrir& selHrir, float gain = 1.0f);

    const float* getOverlapLeft() const { return overlapLeft; }
    int getOverlapSize() const { return overlapSize; }

private:
    static const int MAX_HRIR_SLOTS = 128;
    struct HrirSlot {
        int azimuth;
        HrirData data;
    };

    HrirSlot hrirSlots[MAX_HRIR_SLOTS];
    int hrirCount;
    int sampleRate;
    int blockSize;

    // Buffers pour overlap-add (la taille maximale est MAX_HRIR_LENGTH-1)
    float overlapLeft[MAX_HRIR_LENGTH];
    float overlapRight[MAX_HRIR_LENGTH];
    int overlapSize;  // nombre de samples actuellement stockés (varie en fonction du HRIR)
};

#endif
