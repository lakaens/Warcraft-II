#include "j1Printer.h"
#include "j1Render.h"

#include "Brofiler\Brofiler.h"

bool j1Printer::Awake(pugi::xml_node&)
{
	return true;
}
bool j1Printer::Start()
{
	return true;
}
bool j1Printer::PreUpdate()
{
	return true;
}
bool j1Printer::Update(float dt)
{
	return true;
}
bool j1Printer::PostUpdate()
{
	BROFILER_CATEGORY("Printer Blit", Profiler::Color::Azure);

	while (!drawingQueue.empty())
	{
		DrawingElem* delem = drawingQueue.top();

		switch (delem->type)
		{
		case DrawingElem::DElemType::SPRITE:
		{
			Sprite* sprite = (Sprite*)delem;
			//SDL_SetTextureColorMod(sprite->texture, sprite->color.r, sprite->color.g, sprite->color.b);
			App->render->Blit(sprite->texture, sprite->pos.x, sprite->pos.y, &sprite->squareToBlit, 1, 1, sprite->angle);
			//SDL_SetTextureColorMod(sprite->texture, 255, 255, 255);
			break;
		}
		case DrawingElem::DElemType::QUAD:
		{
			Quad* quad = (Quad*)delem;
			App->render->DrawQuad(quad->rect, quad->color.r, quad->color.g, quad->color.b, quad->color.a, quad->filled, quad->useCamera);
		}
		}

		drawingQueue.pop();
		delete delem;
	}
	return true;
}

bool j1Printer::CleanUp()
{
	while (!drawingQueue.empty())
	{
		DrawingElem* delem = drawingQueue.top();
		drawingQueue.pop();
		delete delem;
	}
	return drawingQueue.empty();
}

bool j1Printer::PrintSprite(iPoint pos, SDL_Texture* texture, SDL_Rect squareToBlit, int layer, float degAngle, SDL_Color color)
{
	Sprite* sprite = new Sprite(pos, texture, squareToBlit, layer, degAngle, color);
	drawingQueue.push(sprite);

	return true;
}

bool j1Printer::PrintQuad(SDL_Rect rect, SDL_Color color, bool filled, bool useCamera, int layer)
{
	Quad* quad = new Quad(rect, color, filled, useCamera, layer);
	drawingQueue.push(quad);

	return true;
}