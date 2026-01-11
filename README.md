# Pathfinding in a Maze - Visualization

A real-time pathfinding visualization using SDL2, OpenGL, and OpenCL. This project demonstrates wavefront-based pathfinding on GPU-accelerated mazes.

## Features

- **GPU-Accelerated Pathfinding**: Uses OpenCL compute kernels for efficient wavefront expansion
- **OpenGL Visualization**: Real-time rendering of maze and pathfinding progress
- **Dynamic Maze Generation**: Python-based maze generation using Kruskal's or Depth-First Search algorithms
- **Interactive Camera**: Pan and zoom controls to explore the maze
- **Cross-Platform**: Supports Linux and Windows via CMake build system

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

### Windows (Visual Studio)

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

## Project Structure

```
pathfinding_in_a_maze/
├── CMakeLists.txt          # CMake build configuration
├── README.md               # This file
├── assets/                 # Shaders and OpenCL kernels
│   ├── shaders/
│   │   ├── shader.vert    # Vertex shader
│   │   └── shader.frag    # Fragment shader
│   └── kernels/
│       └── step_wavefront.cl  # OpenCL pathfinding kernel
├── scripts/
│   └── gen_maze.py        # Maze generation script
└── src/                   # Source code
    ├── main.cpp           # Entry point
    ├── app/               # Application layer
    ├── graphics/          # OpenGL wrappers
    ├── compute/           # OpenCL wrappers
    ├── maze/              # Maze and pathfinding logic
    ├── platform/          # Platform detection
    └── utils/             # Utilities
```

## How It Works

1. **Maze Generation**: A Python script generates a random maze using either Kruskal's algorithm or depth-first search
2. **GPU Upload**: The maze is uploaded to GPU memory as an OpenCL buffer
3. **Wavefront Expansion**: An OpenCL kernel expands the pathfinding wavefront each frame
4. **Visualization**: OpenGL renders the maze with color-coded distances from the start point
5. **Real-time**: The pathfinding progresses in real-time, allowing you to watch the algorithm work

## Algorithm

The pathfinding uses a **breadth-first search (BFS)** wavefront expansion:

- Each step expands the wavefront to neighboring cells
- Distance to each cell is calculated and stored
- The visualization shows the distance gradient as a color map

## License

This project is provided as-is for educational purposes.

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
