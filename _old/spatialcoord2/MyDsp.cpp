#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767
#define PI 3.14159265358979323846

// Constructeur avec position initiale au centre (0,0)
MyDsp::MyDsp() : 
AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
sine(AUDIO_SAMPLE_RATE_EXACT),
echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
x(0.0), y(0.0)
{
  echo0.setDel(10000);
  echo0.setFeedback(0.5);
  echo1.setDel(7000);
  echo1.setFeedback(0.25);
}

MyDsp::~MyDsp(){}

// Mise à jour de la position (x, y)
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, -1.0, 1.0);
  y = constrain(yValue, -1.0, 1.0);

}

// set sine wave frequency
void MyDsp::setFreq(float freq){
  sine.setFrequency(freq);
}

// Fonction principale update() qui traite l’audio en temps réel
void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate(); 
    if (!outBlock[channel]) {
      exit(-1);
    }
  }
      
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    
    // Calcul de l'angle pour déterminer le panning
    float angle = atan2(y, x);  // Angle entre -π et π
    float gainGauche = (cos(angle) + 1.0f) / 2.0f;  // Transforme [-1,1] en [0,1]
    float gainDroit  = (sin(angle) + 1.0f) / 2.0f;  // Transforme [-1,1] en [0,1]

    // Normalisation des gains pour éviter des variations de volume
    float totalGain = sqrt(gainGauche * gainGauche + gainDroit * gainDroit);
    if (totalGain > 0) {
        gainGauche /= totalGain;
        gainDroit /= totalGain;
    }

    // Calcul de l'atténuation en fonction de la distance
    float distance = sqrt(x * x + y * y);
    float attenuation = 1.0f / (1.0f + distance);

    for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {  
      float currentSample;
      
      if (channel == 0) { 
        currentSample = echo0.tick(sineSample) * 0.25 * gainGauche * attenuation; // Écho gauche avec panning
      } else { 
        currentSample = echo1.tick(sineSample) * 0.25 * gainDroit * attenuation; // Écho droit avec panning
      }

      currentSample = max(-1.0f, min(1.0f, currentSample)); // Clamp entre -1 et 1
      int16_t val = currentSample * MULT_16;
      outBlock[channel]->data[i] = val;
    }
  }

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
