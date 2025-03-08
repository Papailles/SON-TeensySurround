#!/usr/bin/env python3
# extractSofaToWav.py
import os
import numpy as np
import pysofaconventions as pysofa
import soundfile as sf

def main():
    """
    Extracts HRIR (Head-Related Impulse Response) data from a SOFA file
    and saves each measurement as a stereo WAV file.

    The script loads the SOFA file, extracts impulse response data, and writes
    WAV files corresponding to each measurement with appropriate filenames.
    """
    # Input SOFA file
    sofa_filename = "assets/hrtf_nh2.sofa"
    
    # Output directory
    output_dir = "assets/hrtfExtract"
    os.makedirs(output_dir, exist_ok=True)

    # Load the SOFA file
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    sampleRate = sofa.getSamplingRate()
    print("File:", sofa_filename)
    print("Sample Rate:", sampleRate)

    # Extract impulse response data
    # IR_data shape: (M, 2, N) -> M measurements, 2 channels (left/right), N samples
    IR_data = sofa.getDataIR()
    M, R, N = IR_data.shape
    print(f"Measurements: M={M}, Channels={R}, HRIR Size={N}")

    # Extract source positions (M,3): (azimuth, elevation, distance)
    source_positions = sofa.getVariableValue("SourcePosition")

    for m in range(M):
        # Extract left and right HRIR
        left = IR_data[m, 0, :]
        right = IR_data[m, 1, :]

        # Retrieve azimuth, elevation, and distance
        az_deg = source_positions[m, 0]
        el_deg = source_positions[m, 1]
        dist = source_positions[m, 2]

        # Construct the filename
        az_str = f"{int(round(az_deg)):03d}"
        el_str = f"{int(round(el_deg)):03d}"
        filename = f"az{az_str}_el{el_str}.wav"
        filepath = os.path.join(output_dir, filename)

        # Create a stereo array (N, 2)
        stereo_data = np.column_stack((left, right))

        # Write to WAV file (32-bit float to preserve dynamics)
        sf.write(filepath, stereo_data, int(sampleRate), subtype='FLOAT')

        print(f"[{m+1}/{M}] => {filename} | az={az_deg:.1f} deg, el={el_deg:.1f} deg, dist={dist:.2f}m")

    print("WAV extraction completed. Files saved in:", output_dir)

if __name__ == "__main__":
    main()