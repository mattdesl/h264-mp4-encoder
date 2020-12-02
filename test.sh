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

# Compile a program to force emcc caching
# RUN mkdir -p /tmp/emcc && \
#     cd /tmp/emcc && \
#     printf "#include <iostream>\nint main(){ std::cout << 0; return *new int; }" > build.cpp && \
#     emcc --bind -s DISABLE_EXCEPTION_CATCHING=0 build.cpp && \
#     rm -rf /tmp/emcc
# COPY . .

# RUN mkdir embuild
# WORKDIR /home/user/embuild

# cmake -DCMAKE_TOOLCHAIN_FILE=$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake .
# cmake --build --parallel .

# WORKDIR /home/user
# RUN npm install
# RUN npm run build
