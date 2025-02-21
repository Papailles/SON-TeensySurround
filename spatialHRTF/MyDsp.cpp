#include "MyDsp.h"
#include <Arduino.h>
#include <Audio.h>
#include "hrir_data.h" // HRIR
#define AUDIO_OUTPUTS 2
#define MULT_16 32767

MyDsp::MyDsp()
: AudioStream(0, nullptr),   // Aucune entrée
  sine(AUDIO_SAMPLE_RATE_EXACT) // On init Sine
{
}

void MyDsp::begin()
{
    // On initialise le moteur HRTF
    hrtfEngine.init(AUDIO_SAMPLE_RATE_EXACT, AUDIO_BLOCK_SAMPLES);

    //hrtfEngine.addHrir(0,   hrirAz0Left,  hrirAz0Right, 2, 5, 128);
    //hrtfEngine.addHrir(30,  hrirAz30Left, hrirAz30Right,4, 7, 128);
    //hrtfEngine.addHrir(60,  hrirAz60Left, hrirAz60Right, 6, 9, 128);
    //hrtfEngine.addHrir(90,  hrirAz90Left, hrirAz90Right, 8, 11, 128);

    // Initialisation de la graine aléatoire (ex. lecture d'une entrée analogique)
    //randomSeed(analogRead(0));

    // 1) Charger depuis le fichier bin
    // Assurez-vous d'avoir initialisé la SD avant (SD.begin(...))
    // ex: hrtfEngine.loadFromBin("/hrtf_nh2.bin");
    // si le fichier est sur la SD

    if(!hrtfEngine.loadFromBin("/hrtf_nh2.bin")) {
        Serial.println("Echec du loadFromBin => fallback manuel?");
        // fallback: on peut faire addHrir(...) manuellement si on veut
        // ...
    }
    else {
        Serial.println("OK => HRIR depuis bin!");
    }
}

void MyDsp::setFreq(float f)
{
    sine.setFrequency(f);
}


void MyDsp::update() {
    // On alloue 2 canaux de sortie (stéréo)
    audio_block_t* outBlock[AUDIO_OUTPUTS];
    for(int c=0; c<AUDIO_OUTPUTS; c++){
        outBlock[c] = allocate();
        if(!outBlock[c]) return; // plus de mémoire
    }

    // Générer un bloc de bruit blanc en float
    float blockMono[AUDIO_BLOCK_SAMPLES];
    for (int i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
        float noiseSample = ((float)random(-32768, 32767)) / 32768.0f;
        blockMono[i] = noiseSample * 0.5f;
    }

    // On choisit un angle
    static int angle = 0;
    angle++;
    if(angle > 360) angle=0;

    // Récupère la HRIR
    auto sel = hrtfEngine.getHrir(angle);

    // Appliquer processBlock
    float leftFloat[AUDIO_BLOCK_SAMPLES];
    float rightFloat[AUDIO_BLOCK_SAMPLES];
    hrtfEngine.processBlock(blockMono, leftFloat, rightFloat, sel);

    // Convertir en int16
    for(int i=0; i<AUDIO_BLOCK_SAMPLES; i++){
        outBlock[0]->data[i] = (int16_t)(leftFloat[i]  * MULT_16);
        outBlock[1]->data[i] = (int16_t)(rightFloat[i] * MULT_16);
    }

    // Transmettre
    transmit(outBlock[0], 0);
    transmit(outBlock[1], 1);

    // Libération
    release(outBlock[0]);
    release(outBlock[1]);
}