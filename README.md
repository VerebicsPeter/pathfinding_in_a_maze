# Pathfinding in a Maze - Visualization

Real-time pathfinding visualization using SDL2, OpenGL, and OpenCL. This project demonstrates wavefront-based pathfinding on GPU-accelerated mazes.

## Prerequisites

### Linux

```bash
# Ubuntu/Debian
sudo apt-get install libsdl2-dev libglew-dev libglm-dev opencl-headers ocl-icd-opencl-dev

# Fedora
sudo dnf install SDL2-devel glew-devel glm-devel opencl-headers ocl-icd-devel

# Arch Linux
sudo pacman -S sdl2 glew glm opencl-headers ocl-icd
```

### Windows

- Install [CMake](https://cmake.org/download/) (3.15 or higher)
- Install [Visual Studio](https://visualstudio.microsoft.com/) (2019 or later)
- Install dependencies via [vcpkg](https://vcpkg.io/):

  ```cmd
  vcpkg install sdl2 glew glm opencl
  ```

### Common Requirements

- **Python 3**: Required for maze generation
  - `numpy` and `matplotlib` packages

  ```bash
  pip install numpy matplotlib
  ```

## Building

### Linux

```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Windows

```cmd
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path to vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

## Running

After building, the executable will be in the `build` directory:

```bash
./pathfinding
```

The program will:

1. Generate a maze using the Python script
2. Initialize OpenGL and OpenCL contexts
3. Start the visualization

## Controls

- **W/A/S/D**: Pan camera up/left/down/right
- **Q/E**: Zoom in/out
- **ESC**: Exit application

## Algorithm

The pathfinding uses a **breadth-first search (BFS)** wavefront expansion:

- Each step expands the wavefront to neighboring cells
- Distance to each cell is calculated and stored
- The visualization shows the distance gradient as a color map
- After calculating distances it backtracks following the path defined by the smallest distances

## Troubleshooting

### OpenCL Not Found

Make sure you have OpenCL drivers installed for your GPU:

- **NVIDIA**: Comes with CUDA toolkit
- **AMD**: ROCm (Linux) or AMD GPU drivers (Windows)
- **Intel**: Intel OpenCL runtime

### Shader Compilation Errors

Check that the `assets/shaders/` directory is properly copied to the build directory.

### Python Script Errors

Ensure Python 3 is in your PATH and numpy is installed:

```bash
python3 --version
python3 -c "import numpy; print('OK')"
```
