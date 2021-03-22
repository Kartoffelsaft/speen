#include <fstream>
#include <filesystem>
#include <toml++/toml.h>
#include <string>

#include "config.h"

std::string const settingsFilename = "settings.toml";

std::string const defaultSettings = R"TOML(
[graphics]
resolutionX = 1280
resolutionY = 720
# Possible values: 1, 2, 4, 8, 16 (and also currently not working)
msaa = 1
vsync = true
renderDistance = 3
fieldOfView = 60

shadowMapResolution = 1536

# Possible control settings listed at https://wiki.libsdl.org/SDL_Keycode
# Can be given either as a single "Control" or as multiple ["ControlA", "ControlB", "ControlC"]
[controls]
forward = "W"
back = "S"
left = "A"
right = "D"
up = "R"
down = "F"
)TOML";

Config Config::init() {
    std::filesystem::path settings{settingsFilename};

    if(!std::filesystem::exists(settings)) {
        std::ofstream createSettings(settingsFilename);
        createSettings << defaultSettings;
    }

    toml::table settingsData = toml::parse_file(settingsFilename);

#define GET_SETTING(SETTING_NAME, SETTING_DEFAULT) .SETTING_NAME = [&](){\
return settingsData[SETTING_SECTION][#SETTING_NAME].value_or(SETTING_DEFAULT);\
}()

#define GET_KEYBINDS(ACTION_NAME) .ACTION_NAME = [&](){\
if(auto* keyName = settingsData[SETTING_SECTION][#ACTION_NAME].as_string()) {\
    return std::unordered_set<SDL_Keycode>{{ SDL_GetKeyFromName((**keyName).c_str()) }};\
} else if(auto* keyNames = settingsData[SETTING_SECTION][#ACTION_NAME].as_array()) {\
    std::unordered_set<SDL_Keycode> keys;\
    for(auto& keyNode: *keyNames) if(auto* keyName = keyNode.as_string()) {\
        keys.insert(SDL_GetKeyFromName((**keyName).c_str()));\
    }\
    return keys;\
} else {\
    return std::unordered_set<SDL_Keycode>{};\
}\
}()

    return Config{
        .graphics {
#define SETTING_SECTION "graphics"
            GET_SETTING(resolutionX,    1280),
            GET_SETTING(resolutionY,    720 ),
            GET_SETTING(msaa,           1   ),
            GET_SETTING(vsync,          true),
            GET_SETTING(renderDistance, 3   ),
            GET_SETTING(fieldOfView,    60.0),

            GET_SETTING(shadowMapResolution, 1526),
#undef SETTING_SECTION
        },

        .keybindings {
#define SETTING_SECTION "controls"
            GET_KEYBINDS(forward),
            GET_KEYBINDS(back),
            GET_KEYBINDS(left),
            GET_KEYBINDS(right),
            GET_KEYBINDS(up),
            GET_KEYBINDS(down),
#undef SETTING_SECTION
        }
    };
}

