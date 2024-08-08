#ifndef MUSICA_H
#define MUSICA_H

#include <Arduino.h>

// Notes of the melody in the song
int melody[] = {
    0, 1, 2, 0, 1, 2,
    3, 1, 2, 3, 3, 1, 2, 3,
    0, 1, 0, 2, 0, 3, 1, 2, 0
};

//Melody faces
int melody_faces[] = {
    0, 1, 2, 0, 1, 2,
    3, 1, 2, 3, 3, 1, 2, 3,
    0, 1, 0, 2, 0, 3, 1, 2, 0
};


// Note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
    8, 8, 8, 4, 4, 4,
    8, 8, 4, 4, 4, 8, 8, 4,
    4, 4, 4, 4, 8, 8, 8, 8, 8
};


#endif