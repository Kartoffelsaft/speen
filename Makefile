.RECIPEPREFIX = >

EMBED_MODEL_FILES ?= true

SRC_DIR := ./src
BUILD_DIR := ./.build
SRCS := $(shell find $(SRC_DIR) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)

CPPFLAGS := \
    -Og \
	-g \
	-std=c++20

LDFLAGS := \
    -lSDL2 \
    -lGL \
    -lX11 \
    -ldl \
    -lpthread \
    -lrt \
    bgfx/.build/linux64_gcc/bin/libbgfx-shared-libDebug.so

INCLUDES := \
    -Ibgfx/include \
    -Ibx/include \
    -Ibimg/include \
    -Itinygltf

DEFINES := 

ifeq ($(EMBED_MODEL_FILES), true)
DEFINES := $(DEFINES) -DEMBED_MODEL_FILES
endif

SHADER_SRC_DIR := ./shaders
SHADER_BUILD_DIR := ./shaderBuild
SHADER_VERTEX_SRCS := $(shell find $(SHADER_SRC_DIR) -name vert*.sc)
SHADER_FRAGMENT_SRCS := $(shell find $(SHADER_SRC_DIR) -name frag*.sc)

MODEL_SRC_DIR := ./rawModels
MODEL_BUILD_DIR := ./cookedModels
MODEL_SRCS := $(shell cd $(MODEL_SRC_DIR); find -type f; cd "$OLDPWD")
ifeq ($(EMBED_MODEL_FILES), true)
MODEL_TARGETS := $(addprefix $(MODEL_BUILD_DIR)/,$(addsuffix .h,$(MODEL_SRCS)))
else
MODEL_TARGETS := $(addprefix $(MODEL_BUILD_DIR)/,$(addsuffix .pmdl,$(MODEL_SRCS)))
endif
MODEL_SRCS := $(addprefix $(MODEL_SRC_DIR),$(MODEL_SRCS))

all: bgfx shaders models a.out compile_commands.json

bgfx:
> cd bgfx
> make linux-debug64
> cd ..

shaders:
> mkdir -p $(SHADER_BUILD_DIR)
> bgfx/.build/linux64_gcc/bin/shadercDebug -f shaders/vert.sc          -o shaderBuild/vert.h          --type vertex   -i bgfx/src --bin2c
> bgfx/.build/linux64_gcc/bin/shadercDebug -f shaders/frag.sc          -o shaderBuild/frag.h          --type fragment -i bgfx/src --bin2c
> bgfx/.build/linux64_gcc/bin/shadercDebug -f shaders/vertShadowmap.sc -o shaderBuild/vertShadowmap.h --type vertex   -i bgfx/src --bin2c
> bgfx/.build/linux64_gcc/bin/shadercDebug -f shaders/fragShadowmap.sc -o shaderBuild/fragShadowmap.h --type fragment -i bgfx/src --bin2c


models: $(MODEL_TARGETS)

$(MODEL_BUILD_DIR)/%.h: $(MODEL_SRC_DIR)/%
> mkdir -p $(dir $@)
> cat $< | xxd -i > $@

$(MODEL_BUILD_DIR)/%.pmdl: $(MODEL_SRC_DIR)/%
> mkdir -p $(dir $@)
> ln $< $@

compile_commands.json: $(SRCS)
> echo 'compile_commands.json creation not set up yet'

a.out: $(OBJS)
> $(CXX) $(OBJS) -o $@ $(LDFLAGS) $(INCLUDES)

$(BUILD_DIR)/%.cpp.o: %.cpp
> mkdir -p $(dir $@)
> $(CXX) $(INCLUDES) $(DEFINES) $(CPPFLAGS) -c $< -o $@

clean:
> rm -r .build cookedModels shaderBuild
