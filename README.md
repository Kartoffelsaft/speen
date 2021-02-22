# Speen ~~Game~~

This isn't a game yet, doofus.

## Building

Prerequisites: `SDL2`, `xxd` (usually from `vim`), and probably a few other obvious things I'm forgetting.

```
git clone --recurse-submodules

# copy some .glb file to [this project's directory]/rawModels/mokey.glb
# 3d models are really big and it's easy enough to create one manually for now

make
```

if it complains about missing shader headers, then run `make -B shaders`

Keep in mind that `build.sh` won't work until after you put a model because of my dumbass build system.
