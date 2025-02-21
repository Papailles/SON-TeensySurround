#ifndef PROJECT_HRTF_ENGINE_H
#define PROJECT_HRTF_ENGINE_H

#include <Arduino.h>  // ou <stdint.h> si tu préfères
#include <stddef.h>   // pour size_t

// Longueur maximale d'une HRIR
static const int MAX_HRIR_LENGTH = 128;

// Structure de stockage basique d'une HRIR
struct HrirData {
    unsigned delayLeft;   // en échantillons
    unsigned delayRight;  // en échantillons
    float left[MAX_HRIR_LENGTH];
    float right[MAX_HRIR_LENGTH];
    size_t length;
};

// Structure retournée quand on sélectionne une HRIR
struct SelectedHrir {
    const float* left;
    const float* right;
    unsigned delayLeft;
    unsigned delayRight;
    size_t length;
};

// Classe minimaliste gérant quelques HRIR et la convolution
class ProjectHrtfEngine
{
public:
    ProjectHrtfEngine();  

    // Initialisation : sampleRate, blockSize (ex : 44100, 128)
    void init(int sRate, int bSize);

    // Ajout d'une HRIR associée à un azimuth
    void addHrir(int azimuthDeg,
                 const float* left, const float* right,
                 unsigned delayLeft, unsigned delayRight,
                 size_t length);

    // Charger un fichier .bin (avec magic="HRIR", etc.)
    bool loadFromBin(const String &filename);  // NOUVEAU

    // Sélection de la HRIR la plus proche de azimuthDeg
    SelectedHrir getHrir(int azimuthDeg);

    // Convolution naïve : in -> outLeft/outRight (bloc de taille = blockSize)
    void processBlock(const float* in, float* outLeft, float* outRight,
                      const SelectedHrir& selHrir);

private:
    // On stocke quelques HRIR (MAX_HRIR_SLOTS)
    static const int MAX_HRIR_SLOTS = 2048; // on peut augmenter si gros .bin
    struct HrirSlot {
        int azimuth;
        HrirData data;
    };

    HrirSlot hrirSlots[MAX_HRIR_SLOTS];
    int hrirCount;

    int sampleRate;
    int blockSize;
};

#endif
