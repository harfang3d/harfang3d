import argparse
import re
import os
import pathlib

parser = argparse.ArgumentParser(description="Rename Python wheel file...")
parser.add_argument('-src', type=str, help="Input directory")
args = parser.parse_args()

input_directory = pathlib.Path(args.src)
pattern = "harfang*.whl"
regex = r"harfang-([0-9]+\.[0-9]+\.[0-9]+)-.*-.*-(.*)\.whl"

for whl_file in input_directory.glob(pattern):  
	os.rename(str(whl_file), re.sub(regex, r"harfang-\1-py3-none-\2.whl", str(whl_file)))
