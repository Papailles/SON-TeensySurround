#!/usr/bin/env python3
# extractSofaToBin.py

import os
import struct
import numpy as np
import pysofaconventions as pysofa

def main():
    """
    Converts a SOFA (Spatially Oriented Format for Acoustics) file into a binary HRIR file.

    The script extracts impulse response data from the SOFA file, processes it,
    and writes it into a binary format for further use.
    """
    sofa_filename = "assets/hrtf_nh2.sofa"  # Input SOFA file
    output_bin = "assets/hrtf_nh2.bin"  # Output binary file

    # If you want to truncate HRIRs, set the desired length (e.g., 128 samples):
    TRIM_LEN = 128

    # Load the SOFA file
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    sampleRate = sofa.getSamplingRate()

    # Extract Impulse Response (IR) data
    # IR_data shape: (M, 2, N) -> M measurements, 2 channels (left/right), N samples
    IR_data = sofa.getDataIR()
    M, R, N = IR_data.shape
    print(f"File: {sofa_filename}\n"
          f"Sample Rate: {sampleRate}\n"
          f"Measurements: M={M}, Channels={R}, HRIR Size={N}")

    # Check if truncation is needed
    if TRIM_LEN < N:
        hrirLen = TRIM_LEN
        print(f"Truncating HRIR to {TRIM_LEN} samples.")
    else:
        hrirLen = N

    # Retrieve source positions (azimuth, elevation, distance)
    source_positions = sofa.getVariableValue("SourcePosition")

    # Open binary file for writing
    with open(output_bin, "wb") as f:
        # 1) Write the "HRIR" magic header
        f.write(b"HRIR")  

        # 2) Write sample rate (uint32, little-endian)
        f.write(struct.pack("<I", int(sampleRate)))  

        # 3) Write HRIR length (uint32)
        f.write(struct.pack("<I", hrirLen))

        # 4) Write number of measurements (uint32)
        f.write(struct.pack("<I", M))

        # 5) Loop through each measurement
        for m in range(M):
            # Read azimuth, elevation, and distance (floats)
            az = source_positions[m, 0]
            el = source_positions[m, 1]
            dist = source_positions[m, 2]
            f.write(struct.pack("<fff", az, el, dist))

            # Extract left and right HRIR vectors, truncate if needed
            left = IR_data[m, 0, :hrirLen]
            right = IR_data[m, 1, :hrirLen]

            # Convert left and right HRIRs to float32 binary format and write to file
            left_f32 = np.array(left, dtype=np.float32)
            right_f32 = np.array(right, dtype=np.float32)
            f.write(left_f32.tobytes(order='C'))
            f.write(right_f32.tobytes(order='C'))

    print(f"Binary file generated: {output_bin}")
    print("Format: [ 'HRIR', sampleRate (uint32), hrirLen (uint32), M (uint32),"
          " for each M: (az, el, dist) (3 floats), leftHRIR (hrirLen floats), rightHRIR (hrirLen floats) ]")

if __name__ == "__main__":
    main()