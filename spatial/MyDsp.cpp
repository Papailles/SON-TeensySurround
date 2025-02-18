#include "MyDsp.h"

#define MULT_16 32767

// Constructeur de MyDsp avec initialisation du panning au centre
MyDsp::MyDsp() : 
AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
sine(AUDIO_SAMPLE_RATE_EXACT),
echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
pan(0.5) // Par défaut, son centré
{
  echo0.setDel(10000);
  echo0.setFeedback(0.4);
  echo1.setDel(10000);
  echo1.setFeedback(0.4);
}

MyDsp::~MyDsp(){}

// Met à jour la valeur du panning en fonction du potentiomètre
void MyDsp::setPan(float panValue){
  pan = constrain(panValue, 0.0, 1.0); // Assure que la valeur est bien entre 0 et 1
}

// set sine wave frequency
void MyDsp::setFreq(float freq){
  sine.setFrequency(freq);
}

float minLevel = 0.01;  // Pour éviter un silence total

// Fonction principale update() qui traite l’audio en temps réel
void MyDsp::update(void) {
  audio_block_t* outBlock[AUDIO_OUTPUTS];

  // Allocation des blocs audio
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    outBlock[channel] = allocate(); 
    if (!outBlock[channel]) {
      exit(-1);
    }
  }
      
  // Génération du signal audio
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    
    for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {  
      float currentSample;

      // Application du panning : modulation de l'amplitude selon la valeur de "pan"
      if (channel == 0) { 
        currentSample = echo0.tick(sineSample) * max(minLevel, (1.0 - pan)) * 0.25; // Atténuation selon pan
      } else { 
        currentSample = echo1.tick(sineSample) * max(minLevel, pan) * 0.25;
      }

      // Assurer que les valeurs restent dans l’intervalle [-1, 1]
      currentSample = max(-1, min(1, currentSample));

      // Conversion en 16 bits
      int16_t val = currentSample * MULT_16;
      outBlock[channel]->data[i] = val;
    }
  }

  // Transmission et libération des blocs audio
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
