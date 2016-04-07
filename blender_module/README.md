# Blender module

- written in Python3
- runs as Blender script
- receives hand parameters
- renders estimated pose

## Requirements:

Script runs only in Blender.

- [Blender](https://www.blender.org/download/) with Numpy module
    - Note: you have to build it from source, pre-built binaries doesn't contain Numpy!

## How to:

Open Blender and run this script. This script uses Blender asynchronous timer as workaround to freezed UI. Every X miliseconds the worker method is called (timer event) to grab data from TCP buffer and to update hand pose. Blender UI is refreshed (re-rendered) automatically during script "sleep" time.    
