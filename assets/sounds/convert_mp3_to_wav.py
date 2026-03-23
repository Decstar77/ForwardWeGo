#!/usr/bin/env python3
"""
Convert all .mp3 files in the current directory to .wav format.
Requires: pydub and ffmpeg
  pip install pydub
  sudo apt install ffmpeg  # Linux
  brew install ffmpeg      # macOS
"""

import os
import sys
from pathlib import Path


def convert_mp3_to_wav(mp3_path: Path) -> Path:
    """Convert a single MP3 file to WAV and return the output path."""
    from pydub import AudioSegment

    wav_path = mp3_path.with_suffix(".wav")
    audio = AudioSegment.from_mp3(mp3_path)
    audio.export(wav_path, format="wav")
    return wav_path


def main():
    current_dir = Path(".")
    mp3_files = sorted(current_dir.glob("*.mp3"))

    if not mp3_files:
        print("No .mp3 files found in the current directory.")
        sys.exit(0)

    print(f"Found {len(mp3_files)} MP3 file(s) to convert.\n")

    success, failed = 0, 0

    for mp3_file in mp3_files:
        try:
            print(f"  Converting: {mp3_file.name} ...", end=" ", flush=True)
            wav_file = convert_mp3_to_wav(mp3_file)
            size_kb = wav_file.stat().st_size / 1024
            print(f"Done → {wav_file.name} ({size_kb:,.0f} KB)")
            success += 1
        except Exception as e:
            print(f"FAILED ({e})")
            failed += 1

    print(f"\nFinished: {success} converted, {failed} failed.")


if __name__ == "__main__":
    main()
