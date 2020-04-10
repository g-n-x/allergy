#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_ttf.h>

enum EXIT_CODE {
	EXIT_FAIL,
	EXIT_OK
};

static char* window_title = "cool title";
static const int SWIDTH = 1280;
static const int SHEIGHT = 720;
static const int TARGET_FPS = 1000 / 60;

static SDL_Renderer* renderer = NULL;
static SDL_Window* window = NULL;

static int init(void);
static int load_media(void);
static void deinit(void);

int
init(void){
	int code = EXIT_OK;
	Uint32 img_flags = IMG_INIT_PNG;
	Uint32 render_flags = SDL_RENDERER_ACCELERATED
		| SDL_RENDERER_PRESENTVSYNC;

	/* sdl subsystems */
	if(SDL_Init(SDL_INIT_VIDEO) != 0) goto FAIL_INIT;
	if(!(IMG_Init(img_flags) & img_flags)) goto FAIL_IMG;
	if(TTF_Init() == -1) goto FAIL_TTF;

	/* create window */
	window = SDL_CreateWindow(window_title,
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
			SWIDTH, SHEIGHT, SDL_WINDOW_SHOWN);
	if(!window) goto FAIL_WINDOW;

	/* create renderer */
	renderer = SDL_CreateRenderer(window, -1, render_flags);
	if(!renderer) goto FAIL_RENDERER;

	return code; /* success */

FAIL_RENDERER:
	SDL_Log("failed to create renderer: %s", SDL_GetError());
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
FAIL_WINDOW:
	SDL_Log("failed to create window: %s", SDL_GetError());
	SDL_DestroyWindow(window);
	window = NULL;
FAIL_TTF:
	SDL_Log("failed to init sdl_ttf: %s", TTF_GetError());
	TTF_Quit();
FAIL_IMG:
	SDL_Log("failed to init sdl_img: %s", IMG_GetError());
	IMG_Quit();
FAIL_INIT:
	SDL_Log("failed to init sdl: %s", SDL_GetError());
	SDL_Quit();

	code = EXIT_FAIL;
	return code;
}

int
load_media(void){
	int code = EXIT_OK;
	return code;
}

void
deinit(void){
	SDL_DestroyRenderer(renderer);
	renderer = NULL;
	SDL_DestroyWindow(window);
	window = NULL;

	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

int main(int argc, char* argv[]){
	int quit = 0;
	int frame_count = 0;
	Uint32 delta;
	SDL_Event e;

	if(!init()) goto FAIL_MAIN;
	if(!load_media()) goto FAIL_LOAD_MEDIA;

	/* game loop */
	while(!quit){

		delta = SDL_GetTicks();
		/* check for input */
		while(SDL_PollEvent(&e) != 0){
			if(e.type == SDL_QUIT) quit = 1;
			if(e.key.keysym.sym == SDLK_ESCAPE) quit = 1;
		}

		/* render onto the framebuffer */
		SDL_RenderClear(renderer);
		/* flip framebuffer */
		SDL_RenderPresent(renderer);

		/* delta frame */
		delta = SDL_GetTicks() - delta;
		if(delta < TARGET_FPS)
			SDL_Delay(TARGET_FPS - delta);
		frame_count++;
		SDL_Log("frames: %d", frame_count);
	}

FAIL_LOAD_MEDIA:
FAIL_MAIN:
	deinit();
	return 0;
}
