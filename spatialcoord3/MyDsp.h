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
    void setPosition(float x, float y); // x pour le panning, y pour la distance/atténuation

  private:
    Sine sine;
    Echo echo0;
    Echo echo1;
    float x, y;         // Valeurs cibles pour la spatialisation
    float smoothedX;    // Valeur lissée pour x (panning)
    float smoothedY;    // Valeur lissée pour y (distance)
};

#endif
