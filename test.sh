EMSDK="/projects/code/emsdk"
EMSDK_NODE_BIN="$EMSDK/node/12.9.1_64bit/bin"
EMSCRIPTEN="$EMSDK/upstream/emscripten"
PATH="$EMSDK:$EMSCRIPTEN:$EMSDK_NODE_BIN:${PATH}"
EM_CONFIG="$EMSDK/.emscripten"
EM_PORTS="$EMSDK/.emscripten_ports"
EM_CACHE="$EMSDK/.emscripten_cache"
EMSDK_NODE="$EMSDK_NODE_BIN/node"
EMCC_WASM_BACKEND=1
EMCC_SKIP_SANITY_CHECK=1

mkdir embuild
cd embuild

cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake ..
cmake --build .