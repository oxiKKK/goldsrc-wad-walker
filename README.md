# GoldSrc-WAD-Walker
A walker for the goldsource WAD3/WAD2 format.

# :wrench: Usage
- `-file <path>` specifies the wad file.
- `-d` prints out the information about the wad file and its contents.
- `-e <miplevel>` exports all the textures from the wad file into BMP images. The miplevel is 0 by default.

# :hammer: Compile
The program was compiled using `msvc`, toolset `v142`, windows sdk version `10.0` and `c++20`

# :pencil: TODO
- Switch to GUI rather that CLI.
- Add a feature to combine multiple WAD files.
- Add a feature to add textures into the WAD file.