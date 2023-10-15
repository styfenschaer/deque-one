import subprocess
import sys
from pathlib import Path
from rich import console

def main(file_names):   
    this_path = Path(__file__).parent

    if not file_names:
        files = this_path.glob("bench_*.py")
    else:
        files = [this_path / name for name in file_names]

    c = console.Console()
    for file in map(str, files):
        c.print("\n" + file, style="yellow")
        subprocess.run([sys.executable, "-O", file])


if __name__ == "__main__":
    main(sys.argv[1:])