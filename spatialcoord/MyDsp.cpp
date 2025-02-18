#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767

// Constructeur avec position initialisée à (0,0)
MyDsp::MyDsp() : 
AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
sine(AUDIO_SAMPLE_RATE_EXACT),
echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
x(0.0), y(0.0),
delayIndex(0)
{
  echo0.setDel(10000);
  echo0.setFeedback(0.4);
  echo1.setDel(10000);
  echo1.setFeedback(0.4);
}

MyDsp::~MyDsp(){}

// Mise à jour de la position (x, y)
void MyDsp::setPosition(float xValue, float yValue){
  x = constrain(xValue, -1.0, 1.0); // Contrainte entre -1 et 1
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
    float audioSample = sine.tick();

    // Calcul de l'angle à partir de x, y
    float angle = atan2(y, x); // Angle en radians

    // Calcul des gains pour les canaux gauche et droit (panning sinusoïdal)
    float gainGauche = cos((angle + PI / 2.0f) / 2.0f);
    float gainDroit  = sin((angle + PI / 2.0f) / 2.0f);

    // Calcul de la distance (plus le son est loin, plus il est atténué)
    float distance = sqrt(x * x + y * y);
    float attenuation = exp(-2.0 * distance); // Atténuation basée sur la distance

    // Application des gains et de l'atténuation
    float left = audioSample * gainGauche * attenuation;
    float right = audioSample * gainDroit * attenuation;

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

  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
