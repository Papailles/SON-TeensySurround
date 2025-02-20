#include "MyDsp.h"
#include <Arduino.h>
#include <Audio.h>
#include "hrir_data.h" // HRIR
#define AUDIO_OUTPUTS 2
#define MULT_16 32767

MyDsp::MyDsp()
: AudioStream(0, nullptr),   // Aucune entrée
  sine(AUDIO_SAMPLE_RATE_EXACT) // On init Sine
{
}

void MyDsp::begin()
{
    // On initialise le moteur HRTF
    hrtfEngine.init(AUDIO_SAMPLE_RATE_EXACT, AUDIO_BLOCK_SAMPLES);

    hrtfEngine.addHrir(0,   hrirAz0Left,  hrirAz0Right, 2, 5, 128);
    hrtfEngine.addHrir(30,  hrirAz30Left, hrirAz30Right,4, 7, 128);
}

void MyDsp::setFreq(float f)
{
    sine.setFrequency(f);
}


void MyDsp::update()
{
    // On alloue 2 canaux de sortie (stéréo)
    audio_block_t* outBlock[AUDIO_OUTPUTS];
    for(int c=0; c<AUDIO_OUTPUTS; c++){
        outBlock[c] = allocate();
        if(!outBlock[c]) return; // plus de mémoire
    }

    // 1) Générer un bloc mono en float
    float blockMono[AUDIO_BLOCK_SAMPLES];
    for(int i=0; i<AUDIO_BLOCK_SAMPLES; i++){
        // on utilise la classe Sine
        float val = sine.tick(); // sinus [-1..+1]
        // amplitude par ex. 0.5 si tu veux
        val *= 0.5f;
        blockMono[i] = val;
    }

    // 2) Sélection de la HRIR
    static int angle = 0;
    angle++;
    if(angle>30) angle=0;
    auto selHrir = hrtfEngine.getHrir(angle);

    // 3) Convolution naive
    float leftFloat[AUDIO_BLOCK_SAMPLES];
    float rightFloat[AUDIO_BLOCK_SAMPLES];
    hrtfEngine.processBlock(blockMono, leftFloat, rightFloat, selHrir);

    // 4) Conversion en int16 et stockage
    for(int i=0; i<AUDIO_BLOCK_SAMPLES; i++){
        outBlock[0]->data[i] = (int16_t)(leftFloat[i]  * MULT_16);
        outBlock[1]->data[i] = (int16_t)(rightFloat[i] * MULT_16);
    }

    // 5) Transmission
    transmit(outBlock[0], 0);
    transmit(outBlock[1], 1);

    // 6) Libération
    release(outBlock[0]);
    release(outBlock[1]);
}
