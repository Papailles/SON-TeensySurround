#!/usr/bin/env python3
import struct
import numpy as np
import pysofaconventions as pysofa
from scipy.signal import resample_poly

def main():
    """
    Converts a SOFA (Spatially Oriented Format for Acoustics) file into a binary HRIR file,
    selecting only measurements with an elevation close to 0°.

    The script extracts impulse response data, resamples it to a target sample rate,
    applies normalization and windowing, and writes the processed data to a binary file.
    """
    sofa_filename = "assets/hrtf_nh2.sofa"  # Input SOFA file
    output_bin = "assets/hrtf_elev0.bin"  # Output binary file
    TRIM_LEN = 128  # Number of samples to keep after truncation
    TARGET_SAMPLE_RATE = 44100  # Target sample rate (Hz)

    # Load the SOFA file
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    source_sample_rate = sofa.getSamplingRate()  # Expected to be 48000 Hz
    IR_data = sofa.getDataIR()
    M, R, N = IR_data.shape  # M = measurements, R = channels (left/right), N = samples per HRIR
    source_positions = sofa.getVariableValue("SourcePosition")

    # Resampling ratio
    up = TARGET_SAMPLE_RATE
    down = source_sample_rate  # Typically 48000 Hz in SOFA files

    # Select HRIRs with elevation close to 0° (±1° tolerance)
    elev_tol = 1.0  # Tolerance in degrees
    selected_indices = [m for m in range(M) if abs(source_positions[m, 1]) <= elev_tol]
    selected_indices.sort(key=lambda m: source_positions[m, 0])  # Sort by azimuth

    with open(output_bin, "wb") as f:
        # Write the updated header
        f.write(b"HRIR")  # Magic identifier
        f.write(struct.pack("<I", TARGET_SAMPLE_RATE))  # Sample rate (uint32, little-endian)
        f.write(struct.pack("<I", TRIM_LEN))  # HRIR length (uint32)
        f.write(struct.pack("<I", len(selected_indices)))  # Number of measurements (uint32)

        for m in selected_indices:
            az, el, dist = source_positions[m]
            f.write(struct.pack("<fff", az, el, dist))  # Write azimuth, elevation, and distance (floats)

            # Retrieve left and right HRIRs
            left_original = IR_data[m, 0, :]
            right_original = IR_data[m, 1, :]

            # Apply resampling with an anti-aliasing filter
            left_resampled = resample_poly(left_original, up, down)
            right_resampled = resample_poly(right_original, up, down)

            # Normalize before truncation to avoid clipping
            max_val = max(np.max(np.abs(left_resampled)), np.max(np.abs(right_resampled)))
            if max_val > 0:
                left_resampled /= max_val
                right_resampled /= max_val

            # Apply a window function to smooth truncation and avoid artifacts
            window = np.hanning(TRIM_LEN * 2)[:TRIM_LEN]  # Smooth transition window
            left_trimmed = left_resampled[:TRIM_LEN] * window
            right_trimmed = right_resampled[:TRIM_LEN] * window

            # Convert to float32 and write to file
            f.write(np.array(left_trimmed, dtype=np.float32).tobytes())
            f.write(np.array(right_trimmed, dtype=np.float32).tobytes())

    print(f"Binary file '{output_bin}' generated at {TARGET_SAMPLE_RATE} Hz!")

if __name__ == "__main__":
    main()