#!/usr/bin/env python3
# extractSofaToWav.py
import os
import numpy as np
import pysofaconventions as pysofa
import soundfile as sf

def main():
    # Nom du fichier SOFA à traiter
    sofa_filename = "hrtf_nh2.sofa"
    
    # Nom du répertoire de sortie
    output_dir = "hrtfExtract"
    os.makedirs(output_dir, exist_ok=True)

    # Charger le fichier SOFA
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    sampleRate = sofa.getSamplingRate()
    print("Fichier:", sofa_filename)
    print("SampleRate =", sampleRate)

    # Data.IR a la forme [M, R, N]
    IR_data = sofa.getDataIR()  # shape (M, 2, N) -> M mesures, 2 canaux, N échantillons
    M, R, N = IR_data.shape
    print(f"Mesures M={M}, canaux R={R}, taille HRIR={N}")

    # Positions (M, 3) : (az, el, distance) en degrés/mètres
    source_positions = sofa.getVariableValue("SourcePosition")

    for m in range(M):
        # Extraire la HRIR gauche/droite
        left = IR_data[m, 0, :]
        right= IR_data[m, 1, :]

        # Récupérer az, el, dist
        az_deg = source_positions[m, 0]
        el_deg = source_positions[m, 1]
        dist   = source_positions[m, 2]

        # Construire un nom de fichier
        az_str = f"{int(round(az_deg)):03d}"
        el_str = f"{int(round(el_deg)):03d}"
        filename = f"az{az_str}_el{el_str}.wav"
        filepath = os.path.join(output_dir, filename)

        # Créer tableau stéréo (N, 2)
        stereo_data = np.column_stack((left, right))

        # Écrire en wav (32 bits float pour conserver la dynamique)
        sf.write(filepath, stereo_data, int(sampleRate), subtype='FLOAT')

        print(f"[{m+1}/{M}] => {filename} | az={az_deg:.1f} deg, el={el_deg:.1f} deg, dist={dist:.2f}m")

    print("Extraction WAV terminée. Fichiers dans le dossier:", output_dir)

if __name__ == "__main__":
    main()
