#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767

// Constructeur avec panoramique initial au centre (0.5)
MyDsp::MyDsp() : 
  AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
  sine(AUDIO_SAMPLE_RATE_EXACT),
  echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
  echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
  x(0.5), // 0.5 correspond au centre
  y(0.0)  // non utilisé
{
  echo0.setDel(10000);
  echo0.setFeedback(0.5);
  echo1.setDel(7000);
  echo1.setFeedback(0.25);
}

MyDsp::~MyDsp(){}

// Mise à jour du panoramique
// Ici, x est compris entre 0.0 (gauche) et 1.0 (droite)
// La valeur de y est ignorée
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, 0.0, 1.0);
  y = 0.0;  // on n'utilise pas y pour le panoramique
}

// Définition de la fréquence de la sinusoïde
void MyDsp::setFreq(float freq){
  sine.setFrequency(freq);
}

// Fonction principale qui traite l’audio en temps réel
void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate(); 
    if (!outBlock[channel]) {
      // Si aucun block n'est disponible, on quitte la fonction update
      return;
    }
  }
      
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();

    // Utilisation de x comme valeur de panoramique
    // 0.0 = tout à gauche, 1.0 = tout à droite
    float gainGauche = 1.0f - x;
    float gainDroit  = x;
    
    // Application de l'effet echo et du gain sur chaque canal
    float sampleGauche = echo0.tick(sineSample) * 0.25 * gainGauche;
    float sampleDroit  = echo1.tick(sineSample) * 0.25 * gainDroit;
    
    // Clamp pour s'assurer que l'échantillon reste entre -1 et 1
    sampleGauche = max(-1.0f, min(1.0f, sampleGauche));
    sampleDroit  = max(-1.0f, min(1.0f, sampleDroit));
    
    int16_t valGauche = sampleGauche * MULT_16;
    int16_t valDroit  = sampleDroit * MULT_16;
    
    outBlock[0]->data[i] = valGauche;
    outBlock[1]->data[i] = valDroit;
  }

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
