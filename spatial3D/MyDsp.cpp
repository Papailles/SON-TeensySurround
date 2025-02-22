#include "MyDsp.h"
#include <cmath>

#define MULT_16 32767

// Paramètres pour le modèle ITD : largeur de tête ~18 cm et vitesse du son ~343 m/s.
#define HEAD_WIDTH 0.18f
#define SPEED_OF_SOUND 343.0f

// Constructeur : on initialise x au centre, y à 0 (devant) et z à 0 (au niveau des oreilles)
MyDsp::MyDsp() : 
  AudioStream(AUDIO_OUTPUTS, new audio_block_t*[AUDIO_OUTPUTS]),
  sine(AUDIO_SAMPLE_RATE_EXACT),
  echo0(AUDIO_SAMPLE_RATE_EXACT, 10000),
  echo1(AUDIO_SAMPLE_RATE_EXACT, 7000),
  x(0.5),
  y(0.0),
  z(0.0),
  smoothedX(0.5),
  smoothedY(0.0),
  smoothedZ(0.0),
  lowPassLeftState(0.0f),
  lowPassRightState(0.0f),
  itdBufferIndex(0)
{
  echo0.setDel(10000);
  echo0.setFeedback(0.5);
  echo1.setDel(7000);
  echo1.setFeedback(0.25);
  
  // Initialisation des buffers ITD à zéro
  for (int i = 0; i < ITD_BUFFER_SIZE; i++) {
    itdBufferLeft[i] = 0.0f;
    itdBufferRight[i] = 0.0f;
  }
}

MyDsp::~MyDsp(){}

// setPosition reçoit :
//   x dans [0,1] (0 = gauche, 1 = droite),
//   y dans [-1,1] (1 = devant, -1 = derrière),
//   z dans [-1,1] (1 = au-dessus, -1 = en dessous)
void MyDsp::setPosition(float xValue, float yValue, float zValue){
  x = constrain(xValue, 0.0, 1.0);
  y = constrain(yValue, -1.0, 1.0);
  z = constrain(zValue, -1.0, 1.0);
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
  
  // Lissage smooth pour x, y et z afin d'éviter des transitions abruptes
  const float smoothingFactor = 0.01f;
  smoothedX += smoothingFactor * (x - smoothedX);
  smoothedY += smoothingFactor * (y - smoothedY);
  smoothedZ += smoothingFactor * (z - smoothedZ);
  
  // --- Calcul de l'azimuth horizontal ---
  // On mappe smoothedX de [0,1] à [-1,1]
  float X = (smoothedX - 0.5f) * 2.0f;  // X dans [-1,1]
  // On utilise la valeur absolue de smoothedY pour définir la distance horizontale effective
  float Y_abs = fabs(smoothedY);
  
  // Calcul de l'azimuth horizontal de façon continue :
  // Si Y_abs est très proche de 0, on fixe l'angle à ±90° selon le signe de X
  float azimuth = (Y_abs < 0.001f) 
                    ? ((X >= 0) ? PI/2.0f : -PI/2.0f)
                    : atan2(Y_abs, X);
  
  // Loi de panning à puissance constante :
  // Utilisation d'une rotation de 45° combinée à la moitié de l'azimuth
  float leftGain = cos((PI/4.0f) + (azimuth/2.0f));
  float rightGain = sin((PI/4.0f) + (azimuth/2.0f));
  
  // --- Calcul du délai interaural (ITD) ---
  // ITD (en secondes) = (HEAD_WIDTH / SPEED_OF_SOUND) * sin(azimuth)
  float itdSeconds = (HEAD_WIDTH / SPEED_OF_SOUND) * sin(azimuth);
  // Si smoothedY est négatif (source derrière), on inverse le signe de l'ITD
  if (smoothedY < 0) {
    itdSeconds = -itdSeconds;
  }
  int itdSamples = (int)(fabs(itdSeconds) * AUDIO_SAMPLE_RATE_EXACT + 0.5f);
  int delayLeft = (itdSeconds > 0) ? itdSamples : 0;
  int delayRight = (itdSeconds < 0) ? itdSamples : 0;
  
  // --- Atténuation et filtrage ---
  // Atténuation exponentielle en fonction de la distance front/back (|smoothedY|)
  float attenuation = pow(0.5f, fabs(smoothedY) * 2.0f);
  
  // Filtrage passe-bas : baseAlpha dépend de la distance, modulé par la hauteur (smoothedZ)
  float verticalEffect = (smoothedZ < 0)
                         ? (1.0f - 0.3f * fabs(smoothedZ))   // effet plus marqué pour une source en dessous
                         : (1.0f - 0.1f * fabs(smoothedZ));  // effet moins marqué pour une source au-dessus
  float baseAlpha = 0.1f + 0.8f * (1.0f - fabs(smoothedY));
  float alpha = baseAlpha * verticalEffect;
  
  // --- Traitement des échantillons ---
  for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    float sineSample = sine.tick();
    float echoOut0 = echo0.tick(sineSample);
    float echoOut1 = echo1.tick(sineSample);
    float processedSample = (echoOut0 + echoOut1) * 0.5f;
    
    // Application du panning horizontal et de l'atténuation (distance front/back)
    float leftSample = processedSample * leftGain * attenuation;
    float rightSample = processedSample * rightGain * attenuation;
    
    // Application d'un filtre passe-bas simple pour simuler la perte de hautes fréquences
    lowPassLeftState = lowPassLeftState + alpha * (leftSample - lowPassLeftState);
    lowPassRightState = lowPassRightState + alpha * (rightSample - lowPassRightState);
    
    float outLeft = lowPassLeftState;
    float outRight = lowPassRightState;
    
    // ----- Application du délai ITD via un buffer circulaire -----
    itdBufferLeft[itdBufferIndex] = outLeft;
    itdBufferRight[itdBufferIndex] = outRight;
    
    int readIndexLeft = (itdBufferIndex + ITD_BUFFER_SIZE - delayLeft) % ITD_BUFFER_SIZE;
    int readIndexRight = (itdBufferIndex + ITD_BUFFER_SIZE - delayRight) % ITD_BUFFER_SIZE;
    
    float delayedLeft = itdBufferLeft[readIndexLeft];
    float delayedRight = itdBufferRight[readIndexRight];
    
    itdBufferIndex = (itdBufferIndex + 1) % ITD_BUFFER_SIZE;
    
    // Clamp des valeurs et conversion en int16_t
    delayedLeft = max(-1.0f, min(1.0f, delayedLeft));
    delayedRight = max(-1.0f, min(1.0f, delayedRight));
    
    outBlock[0]->data[i] = delayedLeft * MULT_16;
    outBlock[1]->data[i] = delayedRight * MULT_16;
  }
  
  for (int channel = 0; channel < AUDIO_OUTPUTS; channel++) {
    transmit(outBlock[channel], channel);
    release(outBlock[channel]);
  }
}
