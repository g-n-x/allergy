/*
 * everything was made based on these rules:
 * 1. an animation spritesheet must have only 1 row
 * 2. all sprites must be squares (at least for now)
 */
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_ttf.h>

#define tex_load_failed(condition) \
	if(condition){SDL_Log("failed to load media: %s",IMG_GetError());code=EXIT_FAIL;}

typedef struct {
	float frame_time;
	int total_frames;
	int current_frame;
	SDL_Rect* frames;
} Animation;

typedef struct {
	int x, y, speed;
	SDL_Texture* tex;
	Animation* animations;
} ObjectInfo;

enum EXIT_CODE {
	EXIT_FAIL,
	EXIT_OK
};

static char* window_title = "sli";
static const int SWIDTH = 1280;
static const int SHEIGHT = 720;
static const int TARGET_FPS = 1000 / 60; /* second per 60 frames */
static const int GAME_SCALE = 5; /* multiply every render' scale by 5 */

static SDL_Renderer* renderer = NULL;
static SDL_Window* window = NULL;
static SDL_Surface* icon = NULL;

/* textures */
static SDL_Texture* sli_atlas = NULL;
static SDL_Texture* bg_test = NULL;

/* rectangle arrays */
static SDL_Rect sli_idle[2];
static SDL_Rect sli_walk[4];
static SDL_Rect sli_jump[7];

/* ObjectInfo */
static ObjectInfo Sli;

/* functions */
static int init(void);
static SDL_Texture* load_texture(const char* path);

/* these two are possibly going to be replaced by some animation function */
static inline void load_rectangle(SDL_Rect* r, int w, int sz); /* used to define the SDL_Rect vars */
static void load_rects(void); /* used to group the load_rectangle() */

static int load_media(void);

/* object related functions */
static void create_animation(ObjectInfo oi, int atlas_row, float frame_time); /* TODO */
static void create_object(SDL_Texture* atlas, Animation* anim_set); /* TODO */
static void load_objects(void);
static int render_object(ObjectInfo* oi); /* TODO */
static void deinit(void);

int
init(void){
	int code = EXIT_OK;
	Uint32 img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
	Uint32 render_flags = SDL_RENDERER_ACCELERATED;

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

inline void
load_rectangle(SDL_Rect* r, int w, int sz){
	/* r is the rect to be modifying */
	/* w speficy width of one frame in the atlas */
	/* sz is for the array size */

	for(int i = 0; i < sz; i++){
		r[i].x += w * i;
		r[i].y = 0;
		r[i].w = w;
		r[i].h = w;
	}
}

void
load_rects(void){
	int i;
	/* sli_idle 2 frames */
	load_rectangle(sli_idle, 16, sizeof(sli_idle));
	/* sli_walk 4 frames */
	load_rectangle(sli_walk, 16, sizeof(sli_walk));
	/* sli_jump 7 frames */
	load_rectangle(sli_jump, 16, sizeof(sli_jump));
}

SDL_Texture*
load_texture(const char* path){
	SDL_Texture* new_tex = NULL;
	SDL_Surface* tmp_surf = IMG_Load(path);

	if(tmp_surf){
		new_tex = SDL_CreateTextureFromSurface(renderer, tmp_surf);
		SDL_FreeSurface(tmp_surf);
	}

	return new_tex;
}

int
load_media(void){
	int code = EXIT_OK;

	/* icon */
	icon = IMG_Load("res/test/icon.png");
	tex_load_failed(!icon) else
		SDL_SetWindowIcon(window, icon);

	/* sli stlas */
	sli_atlas = load_texture("res/sli-atlas.png");
	tex_load_failed(!sli_atlas);

	bg_test = load_texture("res/test/wpp_test.jpg");
	tex_load_failed(!bg_test);

	return code;
}

void
load_objects(void){
	Sli.x = 640;
	Sli.y = 360;
	Sli.speed = 4;
}

int
render_object(ObjectInfo *oi){
	int code = EXIT_OK;
	SDL_RenderCopy(renderer, oi->tex, NULL, NULL);
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
	load_objects();

	/* game loop */
	while(!quit){

		delta = SDL_GetTicks();
		/* check for input */
		while(SDL_PollEvent(&e) != 0){
			if(e.type == SDL_QUIT) quit = 1;

			switch(e.key.type){
				case SDL_KEYDOWN:
					if(e.key.keysym.sym == SDLK_ESCAPE) quit = 1;
					break;
			}
		}

		/* render onto the framebuffer */
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, bg_test, NULL, NULL);

		/* flip framebuffer */
		SDL_RenderPresent(renderer);

		/* delta frame (actually, no) */
		delta = SDL_GetTicks() - delta;
		if(delta < TARGET_FPS)
			SDL_Delay(TARGET_FPS - delta);
		frame_count++;
		SDL_Log("frames: %d", frame_count);
	}

FAIL_LOAD_OBJECTS:
FAIL_LOAD_MEDIA:
FAIL_MAIN:
	deinit();
	return 0;
}
