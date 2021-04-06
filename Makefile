.RECIPEPREFIX = >

MAKE_MODE ?= debug
EMBED_MODEL_FILES ?= false
#TODO: somehow work out how to get compilation working on other platforms
# I tried and holy shit visual studio sucks
HOST_OS ?= linux
TARGET_OS ?= linux

HOST_TARGET_OS := $(HOST_OS)_$(TARGET_OS)

ifeq ($(TARGET_OS),linux)
OUT_EXE ?= a.out
else ifeq ($(TARGET_OS),windows)
OUT_EXE ?= a.exe
endif

ifeq ($(HOST_TARGET_OS),linux_linux)
CXX = g++
else ifeq ($(HOST_TARGET_OS),linux_windows)
CXX = x86_64-w64-mingw32-g++
else
$(error $(HOST_OS) to $(TARGET_OS) compilation is not (currently) supported)
endif

ifeq ($(MAKE_MODE),debug)
MODE_FLAGS := \
    -O0 \
    -g
else ifeq ($(MAKE_MODE),release)
MODE_FLAGS := \
    -O3 \
    -g
else
$(error $(MAKE_MODE) is not a valid mode)
endif

SRC_DIR := ./src
BUILD_DIR := ./.build
SRCS := $(shell find $(SRC_DIR) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/$(HOST_TARGET_OS)/$(MAKE_MODE)/%.o)
DEPENDS := $(SRCS:%=$(BUILD_DIR)/%.d)

ifeq ($(HOST_TARGET_OS),linux_linux)
LIB_OBJS := \
    bgfx/.build/linux64_gcc/bin/libbgfx-shared-libDebug.so \
    $(BUILD_DIR)/dear-imgui/imguiLinux.o
else ifeq ($(HOST_TARGET_OS),linux_windows)
LIB_OBJS := \
    ./bgfx/.build/win64_mingw-gcc/bin/libbgfxRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libbxRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libbimgRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libglslangRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libglsl-optimizerRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libspirv-crossRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libspirv-optRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libbimg_decodeRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libbimg_encodeRelease.a \
    ./bgfx/.build/win64_mingw-gcc/bin/libexample-glueRelease.a \
    ./bx/.build/win64_mingw-gcc/bin/libbxRelease.a \
    $(BUILD_DIR)/dear-imgui/imguiWindows.o
endif

CXXFLAGS := \
    -std=c++20

LDFLAGS := \
    -lSDL2 \
    -ldl \
    -lpthread

ifeq ($(TARGET_OS),linux)
LDFLAGS := $(LDFLAGS) \
    -lGL \
    -lX11 \
    -lrt
else ifeq ($(TARGET_OS),windows)
LDFLAGS := $(LDFLAGS) \
    -lopengl32 \
    -lgdi32 \
    -lpsapi \
    -limm32
endif

INCLUDES := \
    -Ibgfx/include \
    -Ibx/include \
    -Ibimg/include \
    -Itinygltf \
    -Itomlplusplus/include \
    -Iimgui

ifeq ($(HOST_TARGET_OS),linux_windows)
INCLUDES := $(INCLUDES) -Ibx/include/compat/mingw
endif

CPPFLAGS := 

ifeq ($(EMBED_MODEL_FILES), true)
CPPFLAGS := $(CPPFLAGS) -DEMBED_MODEL_FILES
endif

SHADER_SRC_DIR := ./shaders
SHADER_BUILD_DIR := ./shaderBuild
SHADER_SRCS := $(filter-out varying.def.sc,$(shell cd $(SHADER_SRC_DIR); find * -name '*.sc'; cd "$OLDPWD"))
SHADER_TARGETS := $(addprefix $(SHADER_BUILD_DIR)/,$(addsuffix .h,$(basename $(SHADER_SRCS))))
SHADER_SRCS := $(addprefix $(SHADER_SRC_DIR)/,$(SHADER_SRCS))

MODEL_SRC_DIR := ./rawModels
MODEL_BUILD_DIR := ./cookedModels
MODEL_SRCS := $(shell cd $(MODEL_SRC_DIR); find -type f; cd "$OLDPWD")
ifeq ($(EMBED_MODEL_FILES), true)
MODEL_TARGETS := $(addprefix $(MODEL_BUILD_DIR)/,$(addsuffix .h,$(MODEL_SRCS)))
else
MODEL_TARGETS := $(addprefix $(MODEL_BUILD_DIR)/,$(addsuffix .pmdl,$(MODEL_SRCS)))
endif
MODEL_SRCS := $(addprefix $(MODEL_SRC_DIR)/,$(MODEL_SRCS))

.PHONY: all clean models
default: all

$(OUT_EXE): $(OBJS)
> $(CXX) $(OBJS) -o $@ $(LIB_OBJS) $(INCLUDES) $(MODE_FLAGS) $(LDFLAGS) 

bgfx/.build/linux64_gcc/bin/libbgfx-shared-libDebug.so:
> cd bgfx
> make linux-debug64
> cd ..

bgfx/.build/win64_mingw-gcc/bin/lib%.a:
> cd bgfx
> pwd
> make mingw-gcc-release64
> cd ..

./bx/.build/win64_mingw-gcc/bin/libbxRelease.a:
> cd bx
> make mingw-gcc-release64
> cd ..

$(BUILD_DIR)/dear-imgui/imgui%.o: $(shell find imgui -maxdepth 1 -name *.cpp)
> mkdir -p $(dir $@)
> cat $^ > $(dir $@)/amalgamated.cpp
> $(CXX) -O3 -o $@ -c $(dir $@)/amalgamated.cpp -Iimgui

$(SHADER_BUILD_DIR)/v%.h: $(SHADER_SRC_DIR)/v%.sc
> mkdir -p $(SHADER_BUILD_DIR)
> bgfx/.build/linux64_gcc/bin/shadercDebug -f $< -o $@ --type vertex -i bgfx/src --bin2c

$(SHADER_BUILD_DIR)/f%.h: $(SHADER_SRC_DIR)/f%.sc
> mkdir -p $(SHADER_BUILD_DIR)
> bgfx/.build/linux64_gcc/bin/shadercDebug -f $< -o $@ --type fragment -i bgfx/src --bin2c

models: $(MODEL_TARGETS)

$(MODEL_BUILD_DIR)/%.h: $(MODEL_SRC_DIR)/%
> mkdir -p $(dir $@)
> cat $< | xxd -i > $@

$(MODEL_BUILD_DIR)/%.pmdl: $(MODEL_SRC_DIR)/%
> mkdir -p $(dir $@)
> ln $< $@

compile_commands.json: $(SRCS)
> @echo 'compile_commands.json creation not set up yet'

-include $(DEPENDS)

$(BUILD_DIR)/$(HOST_TARGET_OS)/$(MAKE_MODE)/%.cpp.o: %.cpp Makefile
> mkdir -p $(dir $@)
> $(CXX) $(INCLUDES) $(CPPFLAGS) $(CXXFLAGS) $(MODE_FLAGS) -MMD -MP -c $< -o $@

all: $(LIB_OBJS) $(SHADER_TARGETS) models $(OUT_EXE) compile_commands.json

clean:
> rm -r .build cookedModels shaderBuild
