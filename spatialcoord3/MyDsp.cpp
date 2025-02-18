#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767
#define PI 3.14159265358979323846

// Constructeur avec panoramique initial au centre (0.5)
MyDsp::MyDsp() : 
  AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
  sine(AUDIO_SAMPLE_RATE_EXACT),
  echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
  echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
  x(0.5), // 0.5 correspond au centre (panoramique)
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
  y = 0.0;
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
    
    // On traite le signal par les deux effets echo, puis on fait la moyenne
    float echoOut0 = echo0.tick(sineSample);
    float echoOut1 = echo1.tick(sineSample);
    float processedSample = (echoOut0 + echoOut1) * 0.5f;
    
    // Loi de panning cos/sin pour une puissance constante :
    // x = 0.0 -> angle = 0  -> gauche = cos(0)=1, droite = sin(0)=0
    // x = 1.0 -> angle = π/2-> gauche = cos(π/2)=0, droite = sin(π/2)=1
    float angle = x * (PI / 2.0f);
    float gainGauche = cos(angle);
    float gainDroit  = sin(angle);
    
    float sampleGauche = processedSample * gainGauche;
    float sampleDroit  = processedSample * gainDroit;
    
    // Clamp des valeurs pour rester entre -1 et 1
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
