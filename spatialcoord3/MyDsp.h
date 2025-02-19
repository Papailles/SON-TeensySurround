#ifndef faust_teensy_h_
#define faust_teensy_h_

#define AUDIO_OUTPUTS 2
#define MAX_DELAY_SAMPLES 10

#include "Arduino.h"
#include "AudioStream.h"
#include "Audio.h"

#include "Sine.h"
#include "Echo.h"

class MyDsp : public AudioStream
{
  public:
    MyDsp();
    ~MyDsp();
    
    virtual void update(void);
    void setFreq(float freq);
    void setPosition(float x, float y); // Gestion de la spatialisation

  private:
    Sine sine;
    Echo echo0;
    Echo echo1;
    float x, y;         // Valeur cible pour le panning
    float smoothedX;    // Valeur interne pour une transition smooth
};

#endif
