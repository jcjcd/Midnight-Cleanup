#pragma once
#include "RendererDLL.h"

class Texture;
class Renderer;

struct ANIMAVISION_DLL VideoTexture
{
	static bool createAPI();
	static void destroyAPI();

	struct InternalData;
	InternalData* internal_data = nullptr;

	bool create(const char* filename, Renderer* renderer);
	void destroy();
	bool update(float dt);

	void pause();
	void resume();
	bool hasFinished();
	std::shared_ptr<Texture> getTexture();
	float getAspectRatio() const;

	Renderer* renderer = nullptr;
};

