# Topology Creator

This topology-creator is a standalone program using the installed sumo and the prebuilt files to generate one NodeContainers.\
The NodeContainer contains all base stations in the area followed by a number of User Equipment with defined traces to follow.


# Installation (skip if in a Devcontainer)

## Linux

To install SUMO first, do the following under Linux.

```bash
sudo add-apt-repository ppa:sumo/stable
sudo apt-get update
sudo apt-get install sumo sumo-tools sumo-doc
```

If the `add-apt-repository` command is not found, install it by running:


```bash
sudo apt update
sudo apt install software-properties-common
```

Now open `~/.bashrc` and add the following line at the bottom.
You can also run this in a terminal to add the variable temporarily. It will only be accessible for the current terminal session.

```bash
export SUMO_HOME="/usr/share/sumo"
```

Make sure, sumo is actually installed in this folder (it should be). Otherwise set `SUMO_HOME` accordingly.

## Libsumo

To interact with a simulation, Traci is _way_ too slow (>20min on 3600 steps), so we want to use libsumo. Libsumo has the same interface as traci.

### x86_64

To install libsumo on x86_64 architecture, you can simply use `pip install libsumo`, but for arm64, you need to build it from source.

### arm64 (M1, M2, ...)

Steps 1-6 follow the official instructions from the [macOS build website](https://sumo.dlr.de/docs/Installing/MacOS_Build.html)

1. Install dependencies for compiling sumo:

```bash
brew update
xcode-select --install
brew install cmake
```

2. Install dependencies for running sumo:

```bash
brew install --cask xquartz
brew install xerces-c fox proj gdal gl2ps
```

3. For libsumo, you need to install the following dependencies:

```bash
brew install python swig eigen pygobject3 gtk+3 adwaita-icon-theme
```

4. Clone the sumo repository into a folder of your choice:

```bash
git clone --recursive https://github.com/eclipse-sumo/sumo
```

and insert the following line into your `.bashrc` or `.zshrc` file:

```bash
export SUMO_HOME="$PWD/sumo"
```

Reload your shell or run `source ~/.bashrc` or `source ~/.zshrc`.

5. Invoke CMake to trigger the configuration process and build sumo:

```bash
cd $SUMO_HOME
cmake -B build .
cd $SUMO_HOME
cmake --build build --parallel $(sysctl -n hw.ncpu)
```

6. To install libsumo, create a build directory:

```bash
cd $SUMO_HOME
mkdir -p out/build-python
cd out/build-python
```

And if you have python installed with brew, try to build libsumo with the following command (make sure to replace the python path with your python path):

```bash
cmake -DPYTHON_EXECUTABLE=/usr/local/bin/python3.12 ../..
```

7. Insert the following line into your `.bashrc` or `.zshrc` file to make sure that python can find the libsumo module:

```bash
export PYTHONPATH="$SUMO_HOME/tools:$PYTHONPATH"
```

8. (Optional) If step 6 doesn't work when loading libsumo at step 9 (python not configured correctly or it doesn't detect your headers dir or ...), try to build libsumo with the following command with the correct paths (make sure to replace the python header and library paths with your python header and library paths):

```bash
cmake -DPYTHON_EXECUTABLE=/opt/homebrew/Frameworks/Python.framework/Versions/3.12/bin/python3.12 -DPYTHON_LIBRARY:FILEPATH=/opt/homebrew/opt/python@3.12/Frameworks/Python.framework/Versions/3.12/lib/libpython3.12.dylib -DPYTHON_INCLUDE_DIR:PATH=/opt/homebrew/opt/python@3.12/Frameworks/Python.framework/Versions/3.12/Headers ../..
```

and insert the following line into your `.bashrc` or `.zshrc` same as mentioned above:

```bash
export PYTHONPATH="$SUMO_HOME/tools:$PYTHONPATH"
```

**Hint:** To make sure libsumo is built, you can check whether the folder `$SUMO_HOME/tools/libsumo` exists and contains the file `_libsumo.so`.

9. After the build, you should be able to load libsumo (if not working, ensure you have done step 7 and 6 or 8 correctly):

```bash
python
>>> import libsumo
>>>
```


# Usage

1. Configure and build CMake.
2. CMake: Set Build Target to defiance-sumo-topology-creator (if you don't want to run all tests).
3. CMake: Run without Debbuging \
    This executes [`sumo-topology-creator.cc`](./sumo-topology-creator.cc).


# Results

This example creates the mentioned NodeContainer. If using the `--output` parameter, the resulting NodeContainer is additionally written
into the `/build/contrib/defiance/examples` folder, you should the corresponding file `nodes.json` there. This file contains the NodeContainer in a json format.


# Common Errors
- getcwd() fails
    - can't fetch current working directory
    - Solution: Close your current terminal and use a new one.
