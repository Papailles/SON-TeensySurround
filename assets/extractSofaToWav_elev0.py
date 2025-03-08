#!/usr/bin/env python3
import os
import numpy as np
import pysofaconventions as pysofa
import soundfile as sf

def main():
    """
    Extracts HRIR (Head-Related Impulse Response) data from a SOFA file,
    filtering only measurements with an elevation close to 0°,
    and saves them as stereo WAV files.

    The script loads the SOFA file, selects HRIRs where the elevation is near 0°,
    and writes WAV files corresponding to each measurement with appropriate filenames.
    """
    sofa_filename = "assets/hrtf_nh2.sofa"  # Input SOFA file
    output_dir = "assets/hrtfExtract"  # Output directory
    os.makedirs(output_dir, exist_ok=True)

    # Load the SOFA file
    sofa = pysofa.SOFAFile(sofa_filename, 'r')
    sampleRate = sofa.getSamplingRate()
    print("Sample Rate:", sampleRate)

    # Extract impulse response data
    # IR_data shape: (M, 2, N) -> M measurements, 2 channels (left/right), N samples
    IR_data = sofa.getDataIR()
    M, R, N = IR_data.shape
    print(f"Measurements: M={M}, Channels={R}, HRIR Size={N}")

    # Extract source positions (M,3): (azimuth, elevation, distance)
    source_positions = sofa.getVariableValue("SourcePosition")

    # Elevation tolerance (in degrees) for selecting HRIRs
    elev_tol = 1.0
    
    for m in range(M):
        el = source_positions[m, 1]
        if abs(el) > elev_tol:
            continue  # Skip measurements that are not close to 0° elevation

        az_deg = source_positions[m, 0]
        el_deg = source_positions[m, 1]
        dist = source_positions[m, 2]

        # Construct filename based on azimuth
        az_str = f"{int(round(az_deg)):03d}"
        el_str = f"{int(round(el_deg)):03d}"
        filename = f"az{az_str}_el{el_str}.wav"
        filepath = os.path.join(output_dir, filename)

        # Create stereo HRIR data
        stereo_data = np.column_stack((IR_data[m, 0, :], IR_data[m, 1, :]))
        
        # Write to WAV file (32-bit float to preserve dynamics)
        sf.write(filepath, stereo_data, int(sampleRate), subtype='FLOAT')
        print(f"Measurement {m}: {filename} (az={az_deg}°, el={el_deg}°, dist={dist}m)")

    print("WAV extraction completed. Files saved in:", output_dir)

if __name__ == "__main__":
    main()