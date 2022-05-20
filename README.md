# Neptune
This program is directed towards rendering 3D graphics downscaled to a pixelated 2D canvas. It is written mainly in C++ with OpenGL
and uses shaders written in GLSL.

# Getting Started

## Prerequisites
- C++ Compiler
- Assimp

## Installation
1. Clone the repository
   ```sh
    git clone https://github.com/nusbog/Neptune
   ```
2. Build the project (G++ example)
   ```sh
    g++ src/* -oNeptune.exe -Wall -Iinclude -Llib -lglfw3 -lgdi32 -lopengl32 -latlaslib -lassimp.dll
   ```
3. Run the output file (Neptune.exe)

# Usage
The program currently only runs on Windows, it contains fundamental drawing tools that can be chosen to the left side of the window.
The color wheel can be opened by pressing the square at the bottom left of the window. 
To render a 3D object, drag and drop a .obj model on the window.

In-depth information on the functionality of the program is described in the [documentation](documentation.md)

# Contribution
No pull requests will be accepted as the project is currently being ported to another framework.

# License
Licensed under the [MIT License](LICENSE)

# Contact
Mail: [henry@kilba.net](henry@kilba.net)