#include "SDL/include/SDL.h"

#include "Defs.h"
#include "p2Log.h"

#include "j1App.h"
#include "j1Render.h"

#include "j1Window.h"


j1Window::j1Window() : j1Module()
{
	window = NULL;
	screenSurface = NULL;
	name.assign("window");
}

// Destructor
j1Window::~j1Window()
{
}

// Called before render is available
bool j1Window::Awake(pugi::xml_node& config)
{
	LOG("Init SDL window & surface");
	bool ret = true;

	icon = config.child("icon").attribute("name").as_string();
	iconSurface = SDL_LoadBMP(icon.data());


	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		LOG("SDL_VIDEO could not initialize! SDL_Error: %s\n", SDL_GetError());
		ret = false;
	}
	else
	{
		//Create window
		Uint32 flags = SDL_WINDOW_SHOWN;
		fullscreen = config.child("fullscreen").attribute("value").as_bool();
		bool borderless = config.child("borderless").attribute("value").as_bool();
		bool resizable = config.child("resizable").attribute("value").as_bool();
		bool fullscreen_window = config.child("fullscreen_window").attribute("value").as_bool();

		width = config.child("resolution").attribute("width").as_int();
		height = config.child("resolution").attribute("height").as_int();
		scale = config.child("resolution").attribute("scale").as_float();

		if (fullscreen)
		{
			flags |= SDL_WINDOW_FULLSCREEN;
		}

		if (borderless)
		{
			flags |= SDL_WINDOW_BORDERLESS;
		}

		if (resizable)
		{
			flags |= SDL_WINDOW_RESIZABLE;
		}

		if (fullscreen_window)
		{
			flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		}

		window = SDL_CreateWindow(App->GetTitle(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, flags);
		SDL_SetWindowIcon(window, iconSurface);

		if (window == NULL)
		{
			LOG("Window could not be created! SDL_Error: %s\n", SDL_GetError());
			ret = false;
		}
		else
		{
			//Get window surface
			screenSurface = SDL_GetWindowSurface(window);
		}
	}

	return ret;
}

// Called before quitting
bool j1Window::CleanUp()
{
	LOG("Destroying SDL window and quitting all SDL systems");

	//Destroy window
	if (window != NULL)
	{
		SDL_DestroyWindow(window);
	}

	//Quit SDL subsystems
	SDL_Quit();
	return true;
}

// Set new window title
void j1Window::SetTitle(const char* newTitle)
{
	//title.create(newTitle);
	SDL_SetWindowTitle(window, newTitle);
}

void j1Window::GetWindowSize(uint& width, uint& height) const
{
	width = this->width;
	height = this->height;
}

float j1Window::GetScale() const
{
	return scale;
}

void j1Window::SetFullscreen() {
	if (fullscreen) {
		fullscreen = false;
		SDL_SetWindowFullscreen(window, SDL_WINDOW_SHOWN);
//		SDL_RenderSetViewport(App->render->renderer, &App->render->viewport);
	}
	else {
		fullscreen = true;
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);
		//SDL_RenderSetViewport(App->render->renderer, &App->render->viewport);
	}
}