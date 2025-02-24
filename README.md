# Projet SON Ethan & Giani

Ce projet a pour objectif de modifier un son appliqué en entrée d'un Teensy + AudioShield en utilisant HRTF (Head-Related Transfer Function)

## 1. Introduction

La HRTF (Head-Related Transfer Function) décrit comment le son est modifié par la forme de la tête, des oreilles et du torse lorsqu'il se déplace d'une source sonore vers nos oreilles. 
Cette fonction permet de modéliser ces filtrages acoustiques pour recréer virtuellement la perception spatiale d’un son, c'est-à-dire la localisation précise de sa provenance.

La HRIR (Head-Related Impulse Response) est en quelque sorte la version temporelle de la HRTF. 
En effet, la HRIR représente la réponse impulsionnelle d'un système acoustique (la tête, les oreilles, et le torse) lorsqu'un son impulsionnel le traverse. 
En appliquant une transformée de Fourier à une HRIR, on obtient la HRTF, qui décrit comment les différentes fréquences d'un son sont modifiées par ce même système. 
Ainsi, la HRIR permet de générer la HRTF, et inversement, la HRTF est la représentation fréquentielle de la HRIR, essentielle pour simuler la perception spatiale dans des applications audio.

L'idée principale de notre projet est donc de réaliser une convolution entre le signal d'entrée et notre base HRIR, pour recréer un son qui "tourne autour de la tête" de l'auditeur.

## 2. Matériel 

Pour réaliser ce projet, nous utilisons un Teensy 4.0 (https://www.pjrc.com/teensy/) accompagné d'un AudioShield (https://www.pjrc.com/store/teensy3_audio.html).
Pour stocker nos musiques d'entrée et notre table HRIR, nous utilisons aussi une carte SD insérée dans l'AudioShield.

A VENIR : Nous aimerions intégrer deux boutons : un pour activer / désactiver l'effet de spatialisation, et un second pour faire un sélecteur basique de musiques.

## 3. Réalisation technique

### 3.1 Pré-traitement

Avant d'arriver à notre fichier HRIR interprétable par notre Teensy, il faut d'abord trouver une base HRIR de référence.

La génération d'une base HRIR repose sur la mesure en environnement contrôlé des réponses impulsionnelles de la tête, 
obtenues en enregistrant les signaux captés par des microphones placés aux positions des oreilles ou sur une tête artificielle. Pour chaque position de la source sonore, 
les réponses pour l'oreille gauche et droite sont mesurées, traitées, normalisées et ajustées afin d'éliminer les artefacts indésirables, 
ce qui permet de constituer un ensemble complet de données couvrant une large gamme d'angles. Ces mesures, accompagnées de leurs métadonnées spatiales telles que l'azimut, 
l'élévation et la distance, sont ensuite organisées dans un fichier au format SOFA (Spatially Oriented Format for Acoustics). 
Ce format standardisé offre une structure cohérente pour stocker et partager les HRIR, facilitant leur intégration dans divers systèmes de spatialisation audio 
et garantissant une reproduction fidèle de la perception spatiale du son.

