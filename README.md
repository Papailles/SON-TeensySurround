# TeensySurround

This project was made in the "SON" course offered at INSA Lyon to third year telecommunication students (3TC). he aim of this course is to help students get started with projects involving embedded real-time audio signal processing.

## 1. Introduction

The Head-Related Transfer Function (HRTF) describes how sound is modified by the shape of the head, ears, and torso as it travels from a sound source to our ears.
This function allows us to model these acoustic filters in order to virtually recreate the spatial perception of sound, enabling precise localization of its source.

The Head-Related Impulse Response (HRIR) is essentially the time-domain counterpart of the HRTF.
Indeed, HRIR represents the impulse response of an acoustic system (the head, ears, and torso) when an impulsive sound passes through it.
By applying a Fourier transform to an HRIR, we obtain the HRTF, which describes how the different frequencies of a sound are modified by this same system.
Thus, the HRIR is used to generate the HRTF, and conversely, the HRTF is the frequency-domain representation of the HRIR, crucial for simulating spatial perception in audio applications.

The main idea behind our project is therefore to perform a convolution between the input audio signal and our HRIR dataset, in order to recreate sound that appears to "move around the listener’s head."

## 2. Hardware 

To carry out this project, we're using a [Teensy 4.0](https://www.pjrc.com/teensy/) coupled with an [AudioShield](https://www.pjrc.com/store/teensy3_audio.html).

We're also using the [audio adaptor board](https://www.pjrc.com/store/teensy3_audio.html) provided by PJRC which integrates a low-power stereo audio codec (NXP Semiconductors SGTL5000) and an SD card reader.

Additionally, we're using an SD card inserted into the audio adaptor board to store our input audio files and the HRIR dataset.

## Repository content

This repository contains 4 folders : 
- `TeensySurround` : the code embedded on the Teensy 
- `assets` : Python scripts for HRIR & Sofa processing
- `music` : .wav files used in the Teensy
- `_old` : previous versions of the project 

## The assets folder

The assets folder contains several Python scripts used to process and convert HRIR data from SOFA (Spatially Oriented Format for Acoustics) (`.sofa`) files into binary (`.bin`) files compatible with the Teensy 4.0 platform:

- `extractSofaToBin.py` : Converts a .sofa file containing HRIR data into a binary .bin format. The script truncates the HRIR data to a specified length and saves essential metadata like sampling rate, azimuth, elevation, and distance.

- `extractSofaToBin_elev0.py` : Similar to extractSofaToBin.py, but specifically extracts and resamples HRIR data with elevations around 0 degrees. It performs normalization and applies a smooth window function to avoid abrupt transitions in audio playback.

- `analyseHRIR.py` : Analyzes a binary .bin HRIR file, extracting and summarizing information such as sampling rate, HRIR length, number of measurements, and detailed azimuth, elevation, distance, and HRIR data, then saves the analysis in a readable text format (results.txt).

- `extractSofaToWav.py` and `extractSofaToWav_elev0.py` : These scripts export HRIR data from a .sofa file into individual .wav files. The second script (_elev0) specifically filters measurements at 0° elevation.

## HRIR input file

The HRIR file we used for this project is `assets/hrtf_nh2.sofa`, available [here](https://sofacoustics.org/data/database/ari/).

This file is from the ARI Database.

The ARI (Acoustics Research Institute) Database is a comprehensive database of HRIR/HRTF measurements provided in the .sofa format. It contains extensive acoustic measurement data captured from various positions around the human head, which are used to simulate accurate spatial audio experiences.

Other notable HRTF databases include CIPIC and Listen, among others.

## Importing and using the project

0. Make sure you have an SD card with the `assets/hrtf_elev0.bin` file and the content of `music` folder placed directly in its root directory. Then, insert the SD card into the audio adaptor’s SD card slot."

1. Follow 'The Teensy Development Framework' at [Inria's website](https://inria-emeraude.github.io/son/lectures/lecture1/#installing-teensyduino) to configure your development environment 

2. Plug headphones in the audio adaptor board

3. Clone this repository and open `TeensySurround/TeensySurround.ino` in Arduino IDE

4. Compile and upload the projet to the Teensy 4.0 board (In the Serial Monitor console, you should have the following message : 'Attente de la connexion de l'interface...')

5. Close Arduino IDE, install the Python dependencies `pip install -r TeensySurround/requirements.txt` and execute the script `TeensySurround/interface.py`

6. On the interface, click the `Connexion au Teensy` button

7. You're set ! 

## Acknowledgements

Special thanks to:

- Romain Michon & Tanguy Risset, for their course and ongoing support throughout the project.
- The entire educational team for their assistance and guidance.
