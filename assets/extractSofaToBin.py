#!/usr/bin/env python3
# extractSofaToBin.py

import os
import struct
import numpy as np
import pysofaconventions as pysofa

def main():
    sofa_filename = "hrtf_nh2.sofa"
    output_bin = "hrtf_nh2.bin"

    # Si tu veux tronquer les HRIR, par ex. 128 :
    TRIM_LEN = 128

    # Charger le fichier SOFA
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    sampleRate = sofa.getSamplingRate()

    # IR_data : shape (M, 2, N)
    IR_data = sofa.getDataIR()
    M, R, N = IR_data.shape
    print(f"Fichier: {sofa_filename}\n"
          f"SampleRate: {sampleRate}\n"
          f"Mesures M={M}, canaux={R}, taille HRIR={N}")

    # Tronque si besoin
    if TRIM_LEN < N:
        hrirLen = TRIM_LEN
        print(f"Tronquons la HRIR à {TRIM_LEN} échantillons.")
    else:
        hrirLen = N

    # Source positions : (M,3) => (az, el, distance)
    source_positions = sofa.getVariableValue("SourcePosition")

    # Ouvre le fichier binaire en écriture
    with open(output_bin, "wb") as f:
        # 1) Ecrire "HRIR"
        f.write(b"HRIR")  

        # 2) Ecrire sampleRate (uint32_t)
        f.write(struct.pack("<I", int(sampleRate)))  # little-endian

        # 3) Ecrire hrirLen (uint32_t)
        f.write(struct.pack("<I", hrirLen))

        # 4) Ecrire M (uint32_t)
        f.write(struct.pack("<I", M))

        # 5) Boucle sur chaque mesure
        for m in range(M):
            # az, el, dist (floats)
            az = source_positions[m, 0]
            el = source_positions[m, 1]
            dist = source_positions[m, 2]
            f.write(struct.pack("<fff", az, el, dist))

            # Extraire les vecteurs gauche/droite, tronquer si needed
            left = IR_data[m, 0, :hrirLen]
            right=IR_data[m, 1, :hrirLen]

            # Ecrire left, puis right en float32
            # => on peut faire un struct.pack("<{}f".format(hrirLen), *left)
            #   ou np.array(..., dtype=np.float32).tobytes()
            left_f32 = np.array(left, dtype=np.float32)
            right_f32= np.array(right,dtype=np.float32)
            f.write(left_f32.tobytes(order='C'))
            f.write(right_f32.tobytes(order='C'))

    print(f"Fichier binaire généré: {output_bin}")
    print("Format = [ 'HRIR', sampleRate(uint32), hrirLen(uint32), M(uint32),"
          " pour chaque M: (az, el, dist)(3 floats), leftHRIR(hrirLen floats), rightHRIR(hrirLen floats) ]")

if __name__=="__main__":
    main()