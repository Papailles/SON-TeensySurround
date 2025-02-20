#include "MyDsp.h"
#include <Arduino.h>
#include "hrir_data.h"  // on inclut nos HRIR fictifs

// Constructeur
MyDsp::MyDsp()
: AudioStream(1, inputQueueArray),
  sine(AUDIO_SAMPLE_RATE_EXACT)    // on init Sine avec la SR
{
}

// setFreq() : on appelle la méthode de l'objet Sine
void MyDsp::setFreq(float f) {
    sine.setFrequency(f);
}

void MyDsp::begin()
{
    // On init le moteur HRTF
    hrtfEngine.init(44100, AUDIO_BLOCK_SAMPLES);

    // Ajout de la HRIR à 0° (delayLeft=2, delayRight=5, length=128)
    hrtfEngine.addHrir(0, hrirAz0Left, hrirAz0Right, 
                       2, 5, 128);

    // Ajout de la HRIR à 30° (delayLeft=4, delayRight=7, length=128)
    hrtfEngine.addHrir(30, hrirAz30Left, hrirAz30Right,
                       4, 7, 128);
}

void MyDsp::update()
{
    audio_block_t* inBlock = receiveReadOnly(0);
    if(!inBlock) return;

    // Allocation des blocs de sortie
    audio_block_t* outLeft = allocate();
    audio_block_t* outRight= allocate();
    if(!outLeft || !outRight){
        if(outLeft) release(outLeft);
        if(outRight)release(outRight);
        release(inBlock);
        return;
    }

    // Convertir en float
    float inFloat[AUDIO_BLOCK_SAMPLES];
    for(int i=0; i<AUDIO_BLOCK_SAMPLES; i++){
        inFloat[i] = inBlock->data[i] / 32768.0f;
    }

    // Choix d'un azimuth à la main
    // (tu peux plus tard calculer en fonction d'un pot ou autre)
    static int angle = 0; 
    // Par exemple, on oscille entre 0 et 30 :
    // on incrémente par 1, puis on revient à 0
    angle++;
    if(angle > 30) angle= 0;

    // Sélection de la HRIR
    auto selHrir = hrtfEngine.getHrir(angle);

    // Appliquer la convolution naive
    float tmpLeft[AUDIO_BLOCK_SAMPLES];
    float tmpRight[AUDIO_BLOCK_SAMPLES];
    hrtfEngine.processBlock(inFloat, tmpLeft, tmpRight, selHrir);

    // Reconversion vers int16_t
    for(int i=0; i<AUDIO_BLOCK_SAMPLES; i++){
        outLeft->data[i]  = (int16_t)(tmpLeft[i]*32767.f);
        outRight->data[i] = (int16_t)(tmpRight[i]*32767.f);
        /*Serial.print("outLeft = ");
        Serial.println(outLeft->data[i]);*/
    }

    /*float maxLeft = 0.0f;
    for(int i=0; i<AUDIO_BLOCK_SAMPLES; i++){
        float v = fabsf(tmpLeft[i]);
        if(v > maxLeft) maxLeft = v;
    }
    Serial.print("maxLeft = ");
    Serial.println(maxLeft);*/

    // Transmission
    transmit(outLeft, 0);
    transmit(outRight,1);
    release(outLeft);
    release(outRight);
    release(inBlock);
}
