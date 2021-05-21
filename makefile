
# Windows
SOURCE=source/platform_win/compiler_translation_unit.cpp
INCLUDE_DIRS=/I"libs\DX\Include" /I"libs\stb"
DX_LIB_PATH=libs/DX/Lib/x64/
LIBS=user32.lib gdi32.lib shell32.lib $(DX_LIB_PATH)dxgi.lib $(DX_LIB_PATH)d3d11.lib $(DX_LIB_PATH)d3dx11.lib $(DX_LIB_PATH)d3dx10.lib
FLAGS=/EHsc /O2 /Fego.exe


win:
	cl $(INCLUDE_DIRS) $(FLAGS) $(SOURCE) $(LIBS)


