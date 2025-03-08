#!/usr/bin/env python3
import struct

FILENAME = "assets/hrtf_elev0.bin"
OUTPUT_FILE = "assets/results.txt"

def analyze_bin_file(filename, output_file):
    """
    Reads and analyzes a binary HRIR file, extracting and saving relevant information.

    Parameters:
        filename (str): The path to the binary file to be analyzed.
        output_file (str): The path to the output text file where results will be stored.
    """
    try:
        with open(filename, "rb") as f, open(output_file, "w") as out:
            # Read the file header
            magic = f.read(4)  # First 4 bytes should contain the magic identifier
            if magic != b"HRIR":
                out.write(f"Invalid file (unrecognized magic): {magic}\n")
                return
            
            # Read metadata values
            sampleRate = struct.unpack("<I", f.read(4))[0]  # Sampling rate (Hz)
            hrirLen = struct.unpack("<I", f.read(4))[0]  # Length of HRIR (samples)
            M = struct.unpack("<I", f.read(4))[0]  # Number of measurements

            # Write metadata to output file
            out.write(f"Sampling Rate        : {sampleRate} Hz\n")
            out.write(f"HRIR Length         : {hrirLen} samples\n")
            out.write(f"Number of Measures  : {M}\n\n")
            out.write("=== HRIR Content ===\n\n")

            # Read and save HRIR data for each measurement
            for m in range(M):
                az, el, dist = struct.unpack("<fff", f.read(12))  # Read azimuth, elevation, and distance
                left_hrir = struct.unpack(f"<{hrirLen}f", f.read(4 * hrirLen))  # Read left HRIR
                right_hrir = struct.unpack(f"<{hrirLen}f", f.read(4 * hrirLen))  # Read right HRIR

                # Write HRIR data to output file
                out.write(f"Measurement {m+1}/{M} :\n")
                out.write(f"  Azimuth   : {az:.2f}\n")
                out.write(f"  Elevation : {el:.2f}\n")
                out.write(f"  Distance  : {dist:.2f} m\n")
                out.write(f"  Left HRIR : {left_hrir}\n")
                out.write(f"  Right HRIR: {right_hrir}\n\n")
    
    except FileNotFoundError:
        print(f"Error: File '{filename}' not found.")
    except Exception as e:
        print(f"Error during file analysis: {e}")

# Automatically execute the script with the defined file
if __name__ == "__main__":
    analyze_bin_file(FILENAME, OUTPUT_FILE)
    print(f"Analysis complete. Results saved in '{OUTPUT_FILE}'")