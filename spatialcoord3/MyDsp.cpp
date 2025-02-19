#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767

// Constructeur avec panoramique initial au centre (0.5)
MyDsp::MyDsp() : 
  AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
  sine(AUDIO_SAMPLE_RATE_EXACT),
  echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
  echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
  x(0.5),         // Valeur cible du panning (0.0 = gauche, 1.0 = droite)
  smoothedX(0.5), // Valeur utilisée pour un changement progressif
  y(0.0)          // Non utilisé pour le moment
{
  echo0.setDel(10000);
  echo0.setFeedback(0.5);
  echo1.setDel(7000);
  echo1.setFeedback(0.25);
}

MyDsp::~MyDsp(){}

// Mise à jour du panning : x correspond à la valeur cible (entre 0.0 et 1.0)
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, 0.0, 1.0);
  y = 0.0;
}

// Définition de la fréquence de la sinusoïde
void MyDsp::setFreq(float freq){
  sine.setFrequency(freq);
}

void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate(); 
    if (!outBlock[channel]) {
      // Si aucun bloc n'est disponible, on quitte l'update
      return;
    }
  }
  
  // Interpolation pour un basculement smooth
  const float smoothingFactor = 0.01f; // Plus petit => transition plus lente et plus douce
  smoothedX += smoothingFactor * (x - smoothedX);
  
  // Loi de panning cos/sin pour une puissance constante :
  // smoothedX = 0.0 => angle = 0    => gainLeft = cos(0)=1,  gainRight = sin(0)=0
  // smoothedX = 1.0 => angle = π/2  => gainLeft = cos(π/2)=0, gainRight = sin(π/2)=1
  float angle = smoothedX * (PI / 2.0f);
  float gainLeft = cos(angle);
  float gainRight = sin(angle);
  
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    float echoOut0 = echo0.tick(sineSample);
    float echoOut1 = echo1.tick(sineSample);
    float processedSample = (echoOut0 + echoOut1) * 0.5f;
    
    float leftSample = processedSample * gainLeft;
    float rightSample = processedSample * gainRight;
    
    // Clamp pour éviter les dépassements
    leftSample = max(-1.0f, min(1.0f, leftSample));
    rightSample = max(-1.0f, min(1.0f, rightSample));
    
    outBlock[0]->data[i] = leftSample * MULT_16;
    outBlock[1]->data[i] = rightSample * MULT_16;
  }
  
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
