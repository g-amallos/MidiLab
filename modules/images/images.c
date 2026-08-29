#include <raylib.h>
#include <stdint.h>
#include <images.h>


struct image {
    const char* path;
    Image image;
};

struct image images[T_IMAGE_END] = {
    [T_IMAGE_MIDILAB_LOGO_32] = {.path="assets/images/midilablogo/midilab_32.png", .image={NULL,}},
    [T_IMAGE_MIDILAB_LOGO_64] = {.path="assets/images/midilablogo/midilab_64.png", .image={NULL,}},
    [T_IMAGE_MIDILAB_LOGO_256] = {.path="assets/images/midilablogo/midilab_256.png", .image={NULL,}},
    [T_IMAGE_MIDILAB_LOGO_1024] = {.path="assets/images/midilablogo/midilab_1024.png", .image={NULL,}},

};

void _loadImage(struct image* img) {
    if (img->path && !(img->image.data)) img->image = LoadImage(img->path);
}

void _unloadImage(struct image* img) {
    if (img->path && img->image.data) {
        UnloadImage(img->image);
        img->image.data = NULL;
    }
}

int imagesInit() {
    for (int i=0; i<T_IMAGE_END; i++) {
        _loadImage(images+i);
    }

    Image tmpImages[4] = {imageGetImage(T_IMAGE_MIDILAB_LOGO_32), imageGetImage(T_IMAGE_MIDILAB_LOGO_64), imageGetImage(T_IMAGE_MIDILAB_LOGO_256), imageGetImage(T_IMAGE_MIDILAB_LOGO_1024)};

    SetWindowIcons(tmpImages, 4);
    return 0;
}

int imagesClose() {
    for (int i=0; i<T_IMAGE_END; i++) {
        _unloadImage(images+i);
    }
    return 0;
}

Image imageGetImage(enum image_title image) {
    if (image>=T_IMAGE_END || image<0) return (Image){NULL,};
    return images[image].image;
}