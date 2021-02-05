#!/bin/sh

cd ./bgfx
make linux-debug64
cd ..

COMPILER_FLAGS=(
    -Og -g -std=c++20
)

LINKER_FLAGS=(
    -lSDL2
    -lGL
    -lX11
    -ldl
    -lpthread
    -lrt
    bgfx/.build/linux64_gcc/bin/libbgfx-shared-libRelease.so
)

INCLUDES=(
    -I`pwd`/bgfx/include
    -I`pwd`/bx/include
    -I`pwd`/bimg/include
    -I`pwd`/tinygltf
)

CPP_SOURCE=(
    src/main.cpp
)

SHADER_COMPILER='./bgfx/.build/linux64_gcc/bin/shadercRelease'

cd ./rawModels
MODEL_DIRECTORIES=`find -type d`
MODEL_FILES=`find -type f`
cd -

for modelDir in ${MODEL_DIRECTORIES[*]}
do
    mkdir -p cookedModels/$modelDir
done

for rawModel in ${MODEL_FILES[*]}
do
    cat rawModels/$rawModel | xxd -i > cookedModels/$rawModel.h
done

mkdir -p shaderBuild
$SHADER_COMPILER -f shaders/vert.sc -o shaderBuild/vert.h --type vertex   -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/frag.sc -o shaderBuild/frag.h --type fragment -i bgfx/src --bin2c

COMMAND="g++ ${COMPILER_FLAGS[*]} ${LINKER_FLAGS[*]} ${INCLUDES[*]} ${CPP_SOURCE[*]}"
sh -c "${COMMAND}"

echo "[" > compile_commands.json
for file in ${CPP_SOURCE[*]}
do
    echo "{ \"directory\": \"`pwd`\", \"command\": \"${COMMAND}\", \"file\": \"`pwd`/${file}\"}" >> compile_commands.json
done
echo "]" >> compile_commands.json
