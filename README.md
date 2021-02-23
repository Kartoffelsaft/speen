# Speen ~~Game~~

This isn't a game yet, doofus.

## Building

Prerequisites: `SDL2`, `xxd` (usually from `vim`), and probably a few other obvious things I'm forgetting.

```
git clone --recurse-submodules
make
```

if it complains about missing shader headers, then run `make -B shaders`

Keep in mind that `build.sh` won't work until after you put a model because of my dumbass build system.
