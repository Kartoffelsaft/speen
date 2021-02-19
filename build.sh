#!/bin/sh

### SETTINGS ###

DO_EMBED_MODELS=false

### ALL THE OTHER STUFF ###

# Go into bgfx's directory and build it

cd ./bgfx
make linux-debug64
cd ..

# Most of these are arrays of arguments for the compiler
# If you aren't using gcc then they may be different

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
    bgfx/.build/linux64_gcc/bin/libbgfx-shared-libDebug.so
)

# Tells the compiler & linker where to find the header files
# the `pwd` just makes it absolute for compile_commands.json
INCLUDES=(
    -I`pwd`/bgfx/include
    -I`pwd`/bx/include
    -I`pwd`/bimg/include
    -I`pwd`/tinygltf
)

# This is empty because everything that might be in here is in an if block
# Basically contains a bunch of switches for the cpp files to change at compile time
CPP_DEFINES=()

# This could just as easily be CPP_SOURCE=`find src -name *.cpp` (which, in the makefile it is)
CPP_SOURCE=(
    src/main.cpp
    src/model.cpp
    src/rendererState.cpp
    src/modelInstance.cpp
    src/entitySystem.cpp
)

SHADER_COMPILER='./bgfx/.build/linux64_gcc/bin/shadercRelease'

# Go into the model directory and get all the models
# Ex: if there is rawModels/mokey.glb and rawModels/trees/palm.glb
# then MODEL_DIRECTORIES is '.' and './trees'
# and MODEL_FILES is 'mokey.glb' and 'trees/palm.glb'

cd ./rawModels
MODEL_DIRECTORIES=`find -type d`
MODEL_FILES=`find -type f`
cd -

# Make sure MODEL_DIRECTORIES are present in cookedModels/
# Prevents later 'no such file or directory' errors

for modelDir in ${MODEL_DIRECTORIES[*]}
do
    mkdir -p cookedModels/$modelDir
done

# Here would be whatever processing needs to be done to the models before putting them in cookedModels

if [ "$DO_EMBED_MODELS" = true ]
then
    # Use xxd to convert all of the files into includable c-style formatted data
    # If you don't have xxd it'd probably be best to set DO_EMBED_MODELS to false
    # and skip this

    CPP_DEFINES="$CPP_DEFINES -DEMBED_MODEL_FILES"
    for rawModel in ${MODEL_FILES[*]}
    do
        cat rawModels/$rawModel | xxd -i > cookedModels/$rawModel.h
    done
else
    # Create links from all of the 'cooked' models to the raw ones, because as of now the data inside them
    # would be identical (the processing is all done in src/model.cpp). Could easily be a copy instead, but
    # using a link might save a bit of storage space.

    for rawModel in ${MODEL_FILES[*]}
    do
        ln rawModels/$rawModel cookedModels/$rawModel.pmdl
    done
fi

# Convert the shaders into stuff that's includable. This is basically an unrolled version of what's above when
# DO_EMBED_MODELS is true, but with shaderc instead of xxd
# This could have been a loop too, but ehh I'm lazy

mkdir -p shaderBuild
$SHADER_COMPILER -f shaders/vert.sc          -o shaderBuild/vert.h          --type vertex   -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/frag.sc          -o shaderBuild/frag.h          --type fragment -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/vertShadowmap.sc -o shaderBuild/vertShadowmap.h --type vertex   -i bgfx/src --bin2c
$SHADER_COMPILER -f shaders/fragShadowmap.sc -o shaderBuild/fragShadowmap.h --type fragment -i bgfx/src --bin2c

# This is where the compiler is actually invoked
# If you aren't making the compile_commands.json file then you could invoke it directly
# instead of saving it to a variable and invoking that

COMMAND="g++ ${COMPILER_FLAGS[*]} ${LINKER_FLAGS[*]} ${INCLUDES[*]} ${CPP_DEFINES[*]} ${CPP_SOURCE[*]}"
sh -c "${COMMAND}"

echo "[" > compile_commands.json
for file in ${CPP_SOURCE[*]}
do
    echo "{ \"directory\": \"`pwd`\", \"command\": \"${COMMAND}\", \"file\": \"`pwd`/${file}\"}," >> compile_commands.json
done
# I'm too lazy to figure out how to remove the last comma
echo "{\"directory\": \"\", \"command\": \"\", \"file\": \"\"}]" >> compile_commands.json
