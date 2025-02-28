#ifndef faust_teensy_h_
#define faust_teensy_h_

#define AUDIO_OUTPUTS 2
#define MAX_DELAY_SAMPLES 10 // Définition du délai interaural max

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
    void setPan(float panValue); // Nouvelle fonction pour gérer le panning
    
  private:
    Sine sine;
    Echo echo0;
    Echo echo1;
    float pan; // Variable de panning (0 = gauche, 1 = droite)
    float delayBuffer[MAX_DELAY_SAMPLES]; // Buffer pour ITD
    int delayIndex; // Index du buffer de délai
};

#endif
