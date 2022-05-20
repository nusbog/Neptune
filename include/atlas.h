#ifndef ATLAS_H
#define ATLAS_H

#include <stdbool.h>

typedef struct Texture Texture;

typedef struct {
	unsigned int w, h;
	int x_offset;
	int y_offset;
	int h_lim;

	unsigned char* data;
} Atlas;


struct Texture {
	int x_atlas_coord;
	int y_atlas_coord;

	float x_tex_coord;
	float y_tex_coord;

	float w_tex_coord;
	float h_tex_coord;

	unsigned int w, h;
	unsigned char* data;
};

Atlas createAtlas(int width, int height, int height_lim);
void appendTextureToAtlas(Texture *src, Atlas *dest);
Texture loadTextureFromMemory(unsigned char *data, int width, int height, Atlas *atlas);
Texture loadTexture(char *file_name, Atlas *atlas);

#endif /* ATLAS_H */