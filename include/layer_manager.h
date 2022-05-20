#ifndef LAYERS_H
#define LAYERS_H

#include <neptune_core.h>
#include <atlas.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

class Layer {
public:
	void draw();
	void create(int xRes, int yRes, Texture *texture, Shader *shader);
	void updateCoords(float xPos, float yPos, float width, float height);
	void blit(int posX, int posY, int width, int height, unsigned char *data);
	void saveToFile();
	// void resize(int xRes, int yRes);

	NImage layer;
private:
	Texture *layerTexture;
	int xRes, yRes;
	unsigned char *layerData;
};

void Layer::draw() {
	layer.draw();
}

void Layer::create(int xRes, int yRes, Texture *texture, Shader *shader) {
    layer.init(0, 0, xRes, xRes);
    layer.setShader(shader);
    layer.updateColor(NColor(45, 50, 80));
    layer.bindTexture(texture);

    this->xRes = xRes;
    this->yRes = yRes;
    layerData = (unsigned char *)calloc(xRes * yRes * 4, 1);
}

void Layer::updateCoords(float xPos, float yPos, float width, float height) {
	layer.updateCoords(xPos, yPos, width, height);
}

void Layer::blit(int posX, int posY, int width, int height, unsigned char *data) {
	if(posX < 0 || posX > xRes || posY < 0 || posY > yRes)
		return;

	layer.blit(posX, posY, width, height, data);

	int write_offset = (xRes * 4 * posY) + (posX * 4);
	memcpy(layerData + write_offset, data, 4);
}

void Layer::saveToFile() {
	stbi_write_png("layer0.png", xRes, yRes, 4, layerData, xRes * 4);
}

#endif /* LAYERS_H */