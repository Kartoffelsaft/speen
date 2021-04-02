.RECIPEPREFIX = >

MAKE_MODE ?= debug
EMBED_MODEL_FILES ?= false

SRC_DIR := ./src
BUILD_DIR := ./.build
SRCS := $(shell find $(SRC_DIR) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/$(MAKE_MODE)/%.o)
DEPENDS := $(SRCS:%=$(BUILD_DIR)/%.d)
LIB_OBJS := \
    bgfx/.build/linux64_gcc/bin/libbgfx-shared-libDebug.so \
    $(BUILD_DIR)/dear-imgui/imgui.so

ifeq ($(MAKE_MODE),debug)
MODE_FLAGS := \
    -O0 \
    -g
else ifeq ($(MAKE_MODE),release)
MODE_FLAGS := \
    -O3 \
    -g
else
@echo "WARNING: "$(MAKE_MODE)" is not a valid mode"
endif

CPPFLAGS := \
    -std=c++20

LDFLAGS := \
    -lSDL2 \
    -lGL \
    -lX11 \
    -ldl \
    -lpthread \
    -lrt

INCLUDES := \
    -Ibgfx/include \
    -Ibx/include \
    -Ibimg/include \
    -Itinygltf \
    -Itomlplusplus/include \
    -Iimgui

DEFINES := 

ifeq ($(EMBED_MODEL_FILES), true)
DEFINES := $(DEFINES) -DEMBED_MODEL_FILES
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

all: $(LIB_OBJS) $(SHADER_TARGETS) models a.out compile_commands.json

bgfx/.build/linux64_gcc/bin/libbgfx-shared-libDebug.so:
> cd bgfx
> make linux-debug64
> cd ..

$(BUILD_DIR)/dear-imgui/imgui.so: $(shell find imgui -maxdepth 1 -name *.cpp)
> mkdir -p $(dir $@)
> g++ -shared -fPIC -O3 -o $@ $^

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

a.out: $(OBJS)
> $(CXX) $(OBJS) -o $@ $(LDFLAGS) $(LIB_OBJS) $(INCLUDES) $(MODE_FLAGS)

-include $(DEPENDS)

$(BUILD_DIR)/$(MAKE_MODE)/%.cpp.o: %.cpp Makefile
> mkdir -p $(dir $@)
> $(CXX) $(INCLUDES) $(DEFINES) $(CPPFLAGS) $(MODE_FLAGS) -MMD -MP -c $< -o $@

clean:
> rm -r .build cookedModels shaderBuild
