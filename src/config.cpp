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
# Possible values: 1, 2, 4, 8, 16
msaa = 4
vsync = true
renderDistance = 3

shadowMapResolution = 1536

# Possible control settings listed at https://wiki.libsdl.org/SDL_Keycode
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

    return Config{
        .graphics {
#define SETTING_SECTION "graphics"
            GET_SETTING(resolutionX,    1280),
            GET_SETTING(resolutionY,    720 ),
            GET_SETTING(msaa,           1   ),
            GET_SETTING(vsync,          true),
            GET_SETTING(renderDistance, 3   ),

            GET_SETTING(shadowMapResolution, 1526),
        },

        .keybindings {
            .forward = SDL_GetKeyFromName((**settingsData["controls"]["forward"].as_string()).c_str()),
            .back    = SDL_GetKeyFromName((**settingsData["controls"]["back"].as_string()).c_str()),
            .left    = SDL_GetKeyFromName((**settingsData["controls"]["left"].as_string()).c_str()),
            .right   = SDL_GetKeyFromName((**settingsData["controls"]["right"].as_string()).c_str()),
            .up      = SDL_GetKeyFromName((**settingsData["controls"]["up"].as_string()).c_str()),
            .down    = SDL_GetKeyFromName((**settingsData["controls"]["down"].as_string()).c_str()),
        }
    };
}

