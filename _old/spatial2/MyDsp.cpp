#include "MyDsp.h"

#define MULT_16 32767

// Constructeur de MyDsp avec initialisation du panning au centre et du buffer ITD
MyDsp::MyDsp() : 
AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
sine(AUDIO_SAMPLE_RATE_EXACT),
echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
pan(0.5), // Par défaut, son centré
delayIndex(0) // Initialisation de l'index du buffer de délai
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

    // Calcul du délai interaural (ITD)
    int delaySamples = pan * MAX_DELAY_SAMPLES;
    float attenuation = exp(-3.0 * abs(pan)); // Calcul de l'atténuation interaurale (ILD)

    float left, right;

    // Application du panning, ITD et ILD
    left = delayBuffer[(delayIndex - delaySamples + MAX_DELAY_SAMPLES) % MAX_DELAY_SAMPLES] * (1.0 - attenuation);
    right = sineSample * attenuation;

    delayBuffer[delayIndex] = sineSample; // Stocke le sample pour le délai
    delayIndex = (delayIndex + 1) % MAX_DELAY_SAMPLES; // Incrémente l'index du buffer ITD

    // Assurer que les valeurs restent dans l’intervalle [-1, 1]
    left = max(-1, min(1, left));
    right = max(-1, min(1, right));

    // Conversion en 16 bits
    int16_t valL = left * MULT_16;
    int16_t valR = right * MULT_16;

    // Stockage dans les buffers de sortie
    outBlock[0]->data[i] = valL;
    outBlock[1]->data[i] = valR;
  }

  // Transmission et libération des blocs audio
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
