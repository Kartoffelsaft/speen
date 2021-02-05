# Speen ~~Game~~

This isn't a game yet, doofus.

## Building

Prerequisites: `SDL2`, `xxd` (usually from `vim`), and probably a few other obvious things I'm forgetting.

```
git submodule sync --recursive  # at least I think this is the command? I'm to lazy to test.

# copy some .glb file to [this project's directory]/rawModels/mokey.glb

./build.sh
```

Keep in mind that `build.sh` won't work until after you put a model because of my dumbass build system.
