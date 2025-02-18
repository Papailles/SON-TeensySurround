#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767
#define PI 3.14159265358979323846

// Constructeur avec position initiale au centre (0.5)
MyDsp::MyDsp() : 
  AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
  sine(AUDIO_SAMPLE_RATE_EXACT),
  x(0.5),  // Position centrale par défaut
  y(0.0)   // Non utilisé pour le moment
{
  // Réglage de la fréquence par défaut (ex : 440 Hz)
  sine.setFrequency(440.0);
}

MyDsp::~MyDsp(){}

// Mise à jour de la position (panning)
// x va de 0.0 (gauche) à 1.0 (droite)
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, 0.0, 1.0);
  y = 0.0;  // on n'utilise pas y pour le panning
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
  
  // Calcul des gains de panning à puissance constante
  // x = 0.0 => angle = 0  => gainGauche = cos(0)=1, gainDroit = sin(0)=0
  // x = 1.0 => angle = π/2 => gainGauche = cos(π/2)=0, gainDroit = sin(π/2)=1
  float angle = x * (PI / 2.0f);
  float gainLeft = cos(angle);
  float gainRight = sin(angle);
  
  // Traitement des échantillons
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    
    // Application du panning
    float leftSample = sineSample * gainLeft;
    float rightSample = sineSample * gainRight;
    
    // Clamp pour s'assurer que l'échantillon reste dans [-1, 1]
    leftSample = max(-1.0f, min(1.0f, leftSample));
    rightSample = max(-1.0f, min(1.0f, rightSample));
    
    // Conversion en valeur 16 bits
    outBlock[0]->data[i] = leftSample * MULT_16;
    outBlock[1]->data[i] = rightSample * MULT_16;
  }
  
  // Transmission et libération des blocs
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
