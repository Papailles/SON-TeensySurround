#!/usr/bin/env python3
import struct

# 🔧 Nom du fichier à analyser (modifiez ici si nécessaire)
FILENAME = "hrtf_elev0.bin"
OUTPUT_FILE = "results.txt"

def analyze_bin_file(filename, output_file):
    try:
        with open(filename, "rb") as f, open(output_file, "w") as out:
            # Lire l'en-tête
            magic = f.read(4)
            if magic != b"HRIR":
                out.write(f"Fichier invalide (magic non reconnu): {magic}\n")
                return
            
            sampleRate = struct.unpack("<I", f.read(4))[0]
            hrirLen = struct.unpack("<I", f.read(4))[0]
            M = struct.unpack("<I", f.read(4))[0]

            out.write(f"Taux d'echantillonnage : {sampleRate} Hz\n")
            out.write(f"Longueur HRIR         : {hrirLen} echantillons\n")
            out.write(f"Nombre de mesures     : {M}\n\n")
            out.write("=== Contenu des HRIR ===\n\n")

            # Lire et enregistrer les données HRIR
            for m in range(M):
                az, el, dist = struct.unpack("<fff", f.read(12))
                left_hrir = struct.unpack(f"<{hrirLen}f", f.read(4 * hrirLen))
                right_hrir = struct.unpack(f"<{hrirLen}f", f.read(4 * hrirLen))

                out.write(f"Mesure {m+1}/{M} :\n")
                out.write(f"  Azimuth   : {az:.2f}\n")
                out.write(f"  Elevation : {el:.2f}\n")
                out.write(f"  Distance  : {dist:.2f} m\n")
                out.write(f"  HRIR Gauche : {left_hrir}\n")
                out.write(f"  HRIR Droit : {right_hrir}\n\n")

    except FileNotFoundError:
        print(f"Erreur : Fichier '{filename}' introuvable.")
    except Exception as e:
        print(f"Erreur lors de l'analyse du fichier : {e}")

# Exécution automatique du script avec le fichier défini
if __name__ == "__main__":
    analyze_bin_file(FILENAME, OUTPUT_FILE)
    print(f"Analyse terminée. Resultats enregistres dans '{OUTPUT_FILE}'")
