md ext
cd ext

rd /S /Q glfw
git clone https://github.com/glfw/glfw.git

rd /S /Q glbinding
git clone https://github.com/cginternals/glbinding.git

rd /S /Q globjects
git clone https://github.com/cginternals/globjects.git

cd ..
md lib
cd lib

rd /S /Q glm
git clone https://github.com/g-truc/glm.git

rd /S /Q imgui
git clone https://github.com/ocornut/imgui.git

rd /S /Q tinyfd
git clone http://git.code.sf.net/p/tinyfiledialogs/code tinyfd

rd /S /Q stb
git clone https://github.com/nothings/stb.git

cd ..