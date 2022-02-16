# DynaMol - Dynamic Visibility-Driven Molecular Surfaces

DynaMol is a fast and easy method for rendering dynamic Gaussian molecular surfaces. It provides high quality and performance, and does not require any preprocessing such as surface extraction.

The technique itself is described in detail in the following publication:

Stefan Bruckner. Dynamic Visibility-Driven Molecular Surfaces. Computer Graphics Forum, vol. 38, no. 2, pp. 317--329, 2019. doi:10.1111/cgf.13640 
https://vis.uib.no/publications/Bruckner-2019-DVM/

We provide the full source code of the method that was used to generate the results in the paper.

## Getting Started

DynaMol was developed under Windows using Microsoft Visual Studio 2022. It uses [CMake](https://cmake.org/) as its build system and does not contain anything else platform-specific, so it should in principle work using other compilers and operating systems, but this has not been tested. Should there be any issues, we are grateful for any input that helps us make the software work on as many platforms as possible.

The easiest way to get started is to use our release package which, in addition to the source code, contains all prebuilt dependencies and binaries for Windows and Visual Studio 2019 (64-Bit). It is available here:

https://github.com/sbruckner/dynamol/releases

## Prerequisites

The project uses [CMake](https://cmake.org/) and relies on the following libraries: 

- [GLFW](https://www.glfw.org/) 3.3 or higher (https://github.com/glfw/glfw.git) for windowing and input support
- [glm](https://glm.g-truc.net/) 0.9.9.5 or higher (https://github.com/g-truc/glm.git) for its math funtionality
- [glbinding](https://github.com/cginternals/glbinding) 3.1.0 or higher (https://github.com/cginternals/glbinding.git) for OpenGL API binding
- [globjects](https://github.com/cginternals/globjects) 2.0.0 or higher (https://github.com/cginternals/globjects.git) for additional OpenGL wrapping
- [Dear ImGui](https://github.com/ocornut/imgui) 1.71 or higher (https://github.com/ocornut/imgui.git) for GUI elements
- [stb](https://github.com/nothings/stb) 2.26 or higher (https://github.com/nothings/stb.git) for PNG loading and saving
- [tinyfiledialogs](https://sourceforge.net/projects/tinyfiledialogs/) 3.3.9 or higher (https://git.code.sf.net/p/tinyfiledialogs/code) for dialog functionality

The project uses vcpkg(https://vcpkg.io) for dependency management, so this should take care of everything.

## Building

If you are using Visual Studio, you can use its integrated CMake support to build and run the project.

When instead building from the command line, run the following commands from the project root folder:

```
mkdir build
cd build
cmake ..
```

After this, you can compile the debug or release versions of the project using 

```
cmake --build --config Debug
```

and/or

```
cmake --build --config Release
```

After building, the executables will be available in the ```./bin``` folder.

## Running

As mentioned above, the program requires that the current working directory is set to the project root folder.

## Usage

After starting the program, a file dialog will pop up and ask you for a Protein Data Bank (PDB) file (see https://www.rcsb.org/). An example file called is located in the ```./dat``` folder. Some basic usage instructions are displayed in the console window.

## Ports

An experimental web version which uses WebGL 2 Compute (see https://www.khronos.org/registry/webgl/specs/latest/2.0-compute/) is available at https://github.com/sbruckner/dynamol-web
Please let me know about any other ports (see https://vis.uib.no/team/bruckner/ for contact information)

## Acknowledgments

The project uses a number of great shaders developed by Michael Mara, Morgan  McGuire, and colleagues. We'd like to thank them for making their outstanding work available. 

- We use the the method by Mara and McGuire for computing tight polyhedral bounds of a 3D sphere under perspective projection. It is based on their source code available at the link below, with only slight adaptations. 

    Michael Mara and Morgan McGuire. 2D Polyhedral Bounds of a Clipped,  Perspective-Projected 3D Sphere. Journal of Computer Graphics Techniques,  vol. 2, no. 2, pp. 70--83, 2013.
    https://research.nvidia.com/publication/2d-polyhedral-bounds-clipped-perspective-projected-3d-sphere 

- For screen space ambient occlusion, we use an implementation of Scalable Ambient Obscurance by McGuire et al. It is based on their source code available at the link below, with only slight adaptations. 

  Morgan McGuire, Michael Mara, and David Luebke. Scalable Ambient Obscurance. Proceedings of ACM SIGGRAPH/Eurographics High-Performance Graphics, pp. 97--103, 2012.
  http://casual-effects.com/research/McGuire2012SAO/AO/ 

- The depth-of-field shaders are also by Morgan McGuire. They are based on the source code available at the link below, with minor adaptations. 

  Morgan McGuire. The Skylanders SWAP Force Depth-of-Field Shader.
  http://casual-effects.blogspot.com/2013/09/the-skylanders-swap-force-depth-of.html 

## License

Copyright (c) 2019, Stefan Bruckner. Released under the [GPLv3 License](LICENSE.md).
Please see https://vis.uib.no/team/bruckner/ for contact information.
