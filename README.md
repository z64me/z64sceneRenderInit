# z64scene

YouTube video:

[![Video demonstration](http://img.youtube.com/vi/IA34gFKKCFk/0.jpg)](http://www.youtube.com/watch?v=IA34gFKKCFk)

## TODO: I never finalized this for release, so you still have to overwrite Kokiri Forest when testing it.

`z64scene` is a mod whose purpose is to extend OoT's scene functionality. Features include:

- Textures that scroll however you like

- Change textures based on flags

- GIF-style texture animation

- Animated color blending

- ~~Cylindrical billboarding~~ use [this patch](https://github.com/z64me/rank_pointlights) instead

- TODO: Easy Lens of Truth meshes (this was going to be accomplished via DE command to DA command to `00`'d matrix to selectively make models visible or invisible by scaling them based on whether Lens of Truth was active)

- ~~TODO: Custom mini-maps~~

- ~~TODO: Mini-map actor squares and icons~~

- ~~TODO: An API that allows you to use these features in custom actors~~

## Credits

**z64.me** - Programming

**rankaisija** - Finalized for release

**Nokaubure** - SharpOcarina implementation

## Showcase

`TODO` A small collection of GIFs showcasing what users have created would be nice.

## How to use

If you're using SharpOcarina, it will patch the rom for you. Skip ahead to the [SharpOcarina guide](TODO).

If you wish to patch manually, grab one compatible with your rom\* from this repo's `patch` folder. You can apply it using [CloudMax's online patcher](https://cloudmodding.com/app/rompatcher).

Alternatively, applying the patch can be part of your `zzrtl` build script, like so:

```C
rom.cloudpatch(0, "path/to/patch.txt");
```

SharpOcarina embeds a rich editor that emulates every feature. [This guide](TODO) will introduce you to them.

If you're a programmer looking to write an editor or emulator, [read the format specification](SPECIFICATION.md).

\* If you can't find a patch for your rom, it is not supported. Pull requests are welcome!

## ~~API for custom actors~~

`NOTE: This was planned but never implemented. It would have allowed embedding texture animations within custom objects. Pointers to the animation data would have been passed into these functions, which would have set up the ram segments for the user with minimal code.`

~~You'll want to grab a `z64scene.h` file from the `include` folder. **It must version-match the patch you're using.** Save it in your `opt/n64/include` folder. Now when you `#include <z64scene.h>` in your custom actor, you can use some extra functions. Each will be covered in brief here.~~

```c
/* initialize a context */
void *z64scene_init(void *block, int size);

/* free a context */
void z64scene_free(void *ctx);

/* prepare ram segments before drawing */
void z64scene_prep(void *ctx);

/* display a colored square at actor's coordinates on mini-map */
void z64scene_map_square(float x, float z, int d, int r, int g, int b);

/* display a 16x16 rgba5551 icon at actor's coordinates on mini-map */
void z64scene_map_icon(float x, float z, void *icon);
```

~~Here is a super simple actor example using this API:~~

```c
/* TODO */
```

## Building from source, adding features

Prerequisite: http://www.z64.me/guides/overlay-environment-setup-windows

And of course, run `make z64scene GAME=oot-debug MODE=ALL`.
