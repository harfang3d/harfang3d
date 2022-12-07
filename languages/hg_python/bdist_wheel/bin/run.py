from pathlib import Path
import subprocess

def run(name:str):
    if name != "assetc":
        raise RuntimeError(f"Unknown tool {name}")
    
    def method(*args):
        _binary_name = Path(name).name
        _prefix = Path(__file__).parent
        _binary_path = ""

        _binary_path = _prefix / Path("assetc") / _binary_name
        if not _binary_path.is_file():
            _binary_path = Path(str(_binary_path) + ".exe")
            if not _binary_path.is_file():
                raise RuntimeError(f"Failed to find binary: {_binary_name}")

        return subprocess.run(args=[_binary_path, *args], capture_output=False)

    return method
