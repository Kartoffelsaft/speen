#!/bin/sh

### SETTINGS ###

DO_EMBED_MODELS=true

### ALL THE OTHER STUFF ###

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

CPP_DEFINES=()

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

if [ "$DO_EMBED_MODELS" = true ]
then
    CPP_DEFINES="$CPP_DEFINES -DEMBED_MODEL_FILES"
    for rawModel in ${MODEL_FILES[*]}
    do
        cat rawModels/$rawModel | xxd -i > cookedModels/$rawModel.h
    done
else
    for rawModel in ${MODEL_FILES[*]}
    do
        # as of now there is no processing to do, so it's just a link for if processing might be needed
        ln rawModels/$rawModel cookedModels/$rawModel.pmdl
    done
fi

mkdir -p shaderBuild
$SHADER_COMPILER -f shaders/vert.sc          -o shaderBuild/vert.h          --type vertex   -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/frag.sc          -o shaderBuild/frag.h          --type fragment -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/vertShadowmap.sc -o shaderBuild/vertShadowmap.h --type vertex   -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/fragShadowmap.sc -o shaderBuild/fragShadowmap.h --type fragment -i bgfx/src --bin2c

COMMAND="g++ ${COMPILER_FLAGS[*]} ${LINKER_FLAGS[*]} ${INCLUDES[*]} ${CPP_DEFINES[*]} ${CPP_SOURCE[*]}"
sh -c "${COMMAND}"

echo "[" > compile_commands.json
for file in ${CPP_SOURCE[*]}
do
    echo "{ \"directory\": \"`pwd`\", \"command\": \"${COMMAND}\", \"file\": \"`pwd`/${file}\"}" >> compile_commands.json
done
echo "]" >> compile_commands.json
