#ifndef HRIR_DATA_H
#define HRIR_DATA_H

#include <Arduino.h>

static const float hrirAz0Left[128] = {
    0.0f, 0.0f, 1.0f, // Dirac à l'indice 2
    0.0f, 0.0f, 0.0f, /* ... */ 0.0f
};

static const float hrirAz0Right[128] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Dirac à 5
    /* ... */ 0.0f
};

// Azimuth 30
static const float hrirAz30Left[128] = {
    0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Dirac à l'indice 4
    0.0f, 0.0f, /* ... */ 0.0f
};

static const float hrirAz30Right[128] = {
    0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // Dirac à 7
    /* ... */ 0.0f
};

#endif
