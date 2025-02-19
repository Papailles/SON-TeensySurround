#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767

// Constructeur avec position initiale au centre et source proche (x = 0.5, y = 0.0)
MyDsp::MyDsp() : 
  AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
  sine(AUDIO_SAMPLE_RATE_EXACT),
  echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
  echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
  x(0.5),         // Valeur cible pour le panning (0.0 = gauche, 1.0 = droite)
  y(0.0),         // Valeur cible pour la distance (0.0 = proche, 1.0 = lointain)
  smoothedX(0.5), // Valeur lissée initiale pour x
  smoothedY(0.0), // Valeur lissée initiale pour y
  lowPassLeftState(0.0f),
  lowPassRightState(0.0f)
{
  echo0.setDel(10000);
  echo0.setFeedback(0.5);
  echo1.setDel(7000);
  echo1.setFeedback(0.25);
}

MyDsp::~MyDsp(){}

// Mise à jour de la position et de la distance
// x (0.0 à 1.0) contrôle le panning et y (0.0 à 1.0) la distance
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, 0.0, 1.0);
  y = constrain(yValue, 0.0, 1.0);
}

void MyDsp::setFreq(float freq){
  sine.setFrequency(freq);
}

void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate();
    if (!outBlock[channel]) {
      return;
    }
  }
  
  // Interpolation smooth pour x et y
  const float smoothingFactor = 0.01f;
  smoothedX += smoothingFactor * (x - smoothedX);
  smoothedY += smoothingFactor * (y - smoothedY);
  
  // Panning à puissance constante basé sur smoothedX
  float angle = smoothedX * (PI / 2.0f);
  float gainLeft = cos(angle);
  float gainRight = sin(angle);
  
  // Atténuation non linéaire pour simuler la distance
  // Ici, on utilise une loi exponentielle pour une décroissance plus marquée
  float attenuation = pow(0.5f, smoothedY * 2.0f);  // y=0 -> 1.0, y=1 -> 0.25
  
  // Détermination du coefficient du filtre passe-bas
  // Plus smoothedY est grand, plus le filtre est "lourd" (alpha faible)
  // Par exemple, pour y proche : alpha ≈ 0.9, pour y lointain : alpha ≈ 0.1
  float alpha = 0.1f + 0.8f * (1.0f - smoothedY);
  
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    float echoOut0 = echo0.tick(sineSample);
    float echoOut1 = echo1.tick(sineSample);
    float processedSample = (echoOut0 + echoOut1) * 0.5f;
    
    // Application du panning et de l'atténuation
    float leftSample = processedSample * gainLeft * attenuation;
    float rightSample = processedSample * gainRight * attenuation;
    
    // Application d'un filtre passe-bas simple pour simuler la perte de hautes fréquences à distance
    lowPassLeftState = lowPassLeftState + alpha * (leftSample - lowPassLeftState);
    lowPassRightState = lowPassRightState + alpha * (rightSample - lowPassRightState);
    
    // Clamp pour éviter les dépassements
    float outLeft = max(-1.0f, min(1.0f, lowPassLeftState));
    float outRight = max(-1.0f, min(1.0f, lowPassRightState));
    
    outBlock[0]->data[i] = outLeft * MULT_16;
    outBlock[1]->data[i] = outRight * MULT_16;
  }
  
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
