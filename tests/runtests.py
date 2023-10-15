import subprocess
import sys
from pathlib import Path


def main(file_names):
    this_path = Path(__file__).parent

    if not file_names:
        files = this_path.glob("test_*.py")
    else:
        files = [this_path / name for name in file_names]

    for file in map(str, files):
        subprocess.run([sys.executable, file])


if __name__ == "__main__":
    main(sys.argv[1:])