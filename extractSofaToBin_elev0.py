#!/usr/bin/env python3
import struct
import numpy as np
import pysofaconventions as pysofa
from scipy.signal import resample_poly

def main():
    sofa_filename = "hrtf_nh2.sofa"
    output_bin = "hrtf_elev0.bin"
    TRIM_LEN = 128
    TARGET_SAMPLE_RATE = 44100  # Nouvelle fréquence cible

    # Charger le fichier SOFA
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    source_sample_rate = sofa.getSamplingRate()  # Devrait être 48000 Hz
    IR_data = sofa.getDataIR()   
    M, R, N = IR_data.shape
    source_positions = sofa.getVariableValue("SourcePosition")

    # Ratio de rééchantillonnage
    up = TARGET_SAMPLE_RATE
    down = source_sample_rate  # 48000

    # Sélectionner les HRIR à élévation 0°
    elev_tol = 1.0
    selected_indices = [m for m in range(M) if abs(source_positions[m, 1]) <= elev_tol]
    selected_indices.sort(key=lambda m: source_positions[m, 0])

    with open(output_bin, "wb") as f:
        # Écrire l'en-tête mis à jour
        f.write(b"HRIR") 
        f.write(struct.pack("<I", TARGET_SAMPLE_RATE))  
        f.write(struct.pack("<I", TRIM_LEN))           
        f.write(struct.pack("<I", len(selected_indices)))              

        for m in selected_indices:
            az, el, dist = source_positions[m]
            f.write(struct.pack("<fff", az, el, dist))

            # Récupérer les HRIR gauche et droit
            left_original = IR_data[m, 0, :]
            right_original = IR_data[m, 1, :]

            # Appliquer un rééchantillonnage en utilisant un filtre passe-bas
            left_resampled = resample_poly(left_original, up, down)
            right_resampled = resample_poly(right_original, up, down)

            # Appliquer une normalisation avant le découpage
            max_val = max(np.max(np.abs(left_resampled)), np.max(np.abs(right_resampled)))
            if max_val > 0:
                left_resampled /= max_val
                right_resampled /= max_val

            # Tronquer en appliquant une fenêtre pour éviter les coupures abruptes
            window = np.hanning(TRIM_LEN * 2)[:TRIM_LEN]  # Fenêtre douce pour éviter les pops
            left_trimmed = left_resampled[:TRIM_LEN] * window
            right_trimmed = right_resampled[:TRIM_LEN] * window

            # Conversion en float32 et écriture
            f.write(np.array(left_trimmed, dtype=np.float32).tobytes())
            f.write(np.array(right_trimmed, dtype=np.float32).tobytes())

    print(f"✅ Fichier '{output_bin}' généré à {TARGET_SAMPLE_RATE} Hz !")

if __name__ == "__main__":
    main()
