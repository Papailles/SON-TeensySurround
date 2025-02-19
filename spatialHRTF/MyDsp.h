#ifndef MY_DSP_H
#define MY_DSP_H

#include "Sine.h"
// Si tu veux inclure le moteur HRTF :
#include "ProjectHrtfEngine.h"
#include <AudioStream.h>


// Classe MyDsp qui hérite de AudioStream
class MyDsp : public AudioStream
{
public:
    // Constructeur
    MyDsp();

    // Méthode pour initialiser (si besoin, ex: chargement HRIR, etc.)
    void begin();

    // setFreq qu'on relie à l'objet Sine
    void setFreq(float f);

    // Méthode appelée par la Teensy Audio Library
    virtual void update();

private:
    // Si tu reçois un bloc en entrée, alors 1. Si tu ne génères que du son, mets 0.
    audio_block_t* inputQueueArray[1];

    // Notre générateur de sinusoïde
    Sine sine;

    // (Optionnel) Moteur HRTF si tu en as besoin
    ProjectHrtfEngine hrtfEngine;

    // etc.
};

#endif
