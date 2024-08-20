import os

directory = ""
output_file = "all_includes.h"

with open(output_file, "w") as f:
    for filename in os.listdir(directory):
        if filename.endswith(".h"):
            f.write(f'#include "{filename}"\n')
