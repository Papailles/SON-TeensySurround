#!/usr/bin/env python3
import os
import numpy as np
import pysofaconventions as pysofa
import soundfile as sf

def main():
    sofa_filename = "hrtf_nh2.sofa"
    output_dir = "hrtfExtract"
    os.makedirs(output_dir, exist_ok=True)

    # Charger le fichier SOFA
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    sampleRate = sofa.getSamplingRate()
    print("SampleRate =", sampleRate)

    IR_data = sofa.getDataIR()  # forme (M, 2, N)
    M, R, N = IR_data.shape
    print(f"Nombre de mesures = {M}, canaux = {R}, taille HRIR = {N}")

    source_positions = sofa.getVariableValue("SourcePosition")  # (M,3): az, el, dist

    elev_tol = 1.0  # tolérance en degrés pour l'élévation
    for m in range(M):
        el = source_positions[m, 1]
        if abs(el) > elev_tol:
            continue  # on ignore si l'élévation n'est pas proche de 0

        az_deg = source_positions[m, 0]
        el_deg = source_positions[m, 1]
        dist   = source_positions[m, 2]

        # Créer un nom de fichier basé sur l'azimut
        az_str = f"{int(round(az_deg)):03d}"
        el_str = f"{int(round(el_deg)):03d}"
        filename = f"az{az_str}_el{el_str}.wav"
        filepath = os.path.join(output_dir, filename)

        stereo_data = np.column_stack((IR_data[m, 0, :], IR_data[m, 1, :]))
        sf.write(filepath, stereo_data, int(sampleRate), subtype='FLOAT')
        print(f"Mesure {m}: {filename} (az={az_deg}°, el={el_deg}°, dist={dist}m)")

    print("Extraction WAV terminée. Les fichiers sont dans", output_dir)

if __name__=="__main__":
    main()
