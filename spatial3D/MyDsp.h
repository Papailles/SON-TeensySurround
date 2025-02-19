#ifndef faust_teensy_h_
#define faust_teensy_h_

#define AUDIO_OUTPUTS 2
#define MAX_DELAY_SAMPLES 10
#define ITD_BUFFER_SIZE 64   // Taille du buffer pour le délai ITD

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
    // Les coordonnées : 
    //   x (0.0 à 1.0) contrôle le panning horizontal,
    //   y (-1.0 à 1.0) contrôle la profondeur (1.0 devant, -1.0 derrière),
    //   z (-1.0 à 1.0) contrôle la hauteur (1.0 au-dessus, -1.0 en dessous)
    void setPosition(float x, float y, float z);

  private:
    Sine sine;
    Echo echo0;
    Echo echo1;
    // Coordonnées cibles
    float x, y, z;
    // Valeurs lissées pour éviter des transitions abruptes
    float smoothedX, smoothedY, smoothedZ;
    // États pour le filtre passe-bas (un pour chaque canal)
    float lowPassLeftState;
    float lowPassRightState;
    
    // Buffers et index pour implémenter le délai ITD
    float itdBufferLeft[ITD_BUFFER_SIZE];
    float itdBufferRight[ITD_BUFFER_SIZE];
    int itdBufferIndex;
};

#endif
