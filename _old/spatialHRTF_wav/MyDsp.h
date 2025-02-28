#ifndef MY_DSP_H
#define MY_DSP_H

#include "ProjectHrtfEngine.h"
#include <AudioStream.h>

#define AUDIO_OUTPUTS 2

class MyDsp : public AudioStream {
public:
    MyDsp();
    void begin();
    virtual void update();

    // Méthodes pour contrôler l'angle de spatialisation
    void setAngle(int newAngle);
    int getAngle() const;

private:
    audio_block_t* inputQueueArray[1];
    ProjectHrtfEngine hrtfEngine;

    // Buffers pour la sortie en float (utilisés par processBlock)
    float outFloatLeft[AUDIO_BLOCK_SAMPLES];
    float outFloatRight[AUDIO_BLOCK_SAMPLES];

    int currentAngle;
};

#endif
