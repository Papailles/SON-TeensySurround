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
  smoothedY(0.0)  // Valeur lissée initiale pour y
{
  echo0.setDel(10000);
  echo0.setFeedback(0.5);
  echo1.setDel(7000);
  echo1.setFeedback(0.25);
}

MyDsp::~MyDsp(){}

// Mise à jour de la position : x et y sont compris entre 0.0 et 1.0
// x contrôle le panning, y contrôle l'atténuation (distance)
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, 0.0, 1.0);
  y = constrain(yValue, 0.0, 1.0);
}

// Définition de la fréquence de la sinusoïde
void MyDsp::setFreq(float freq){
  sine.setFrequency(freq);
}

void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];

  // Allocation des blocs audio pour chaque canal
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate(); 
    if (!outBlock[channel]) {
      return;
    }
  }
  
  // Interpolation smooth pour x et y
  const float smoothingFactor = 0.01f; // Ajuste cette valeur pour des transitions plus lentes ou plus rapides
  smoothedX += smoothingFactor * (x - smoothedX);
  smoothedY += smoothingFactor * (y - smoothedY);
  
  // Calcul du panning à puissance constante pour le canal gauche/droit basé sur smoothedX
  // smoothedX = 0.0 -> angle = 0      -> gainLeft = cos(0)=1, gainRight = sin(0)=0
  // smoothedX = 1.0 -> angle = π/2    -> gainLeft = cos(π/2)=0, gainRight = sin(π/2)=1
  float angle = smoothedX * (PI / 2.0f);
  float gainLeft = cos(angle);
  float gainRight = sin(angle);
  
  // Utilisation de smoothedY pour moduler l'atténuation globale
  // Ici, y = 0 signifie aucune atténuation (source proche) et y = 1 signifie volume très atténué (source lointaine)
  float attenuation = 1.0f - smoothedY;
  
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    float echoOut0 = echo0.tick(sineSample);
    float echoOut1 = echo1.tick(sineSample);
    float processedSample = (echoOut0 + echoOut1) * 0.5f;
    
    float leftSample = processedSample * gainLeft * attenuation;
    float rightSample = processedSample * gainRight * attenuation;
    
    // Clamp des valeurs pour rester dans [-1, 1]
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
