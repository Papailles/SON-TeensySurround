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
    // x contrôle le panning, y la distance/atténuation et le filtrage
    void setPosition(float x, float y); 

  private:
    Sine sine;
    Echo echo0;
    Echo echo1;
    float x, y;          // Valeurs cibles pour la spatialisation
    float smoothedX;     // Valeur lissée pour le panning
    float smoothedY;     // Valeur lissée pour la distance
    // États pour le filtre passe-bas (un pour chaque canal)
    float lowPassLeftState;
    float lowPassRightState;
};

#endif
