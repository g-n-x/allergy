/*
 * everything was made based on these rules:
 * 1. an animation spritesheet must have only 1 row
 * 2. all sprites must be squares (at least for now)
 */
#include<SDL2/SDL.h>
#include<SDL2/SDL_image.h>
#include<SDL2/SDL_ttf.h>

#define tex_load_failed(condition)                          \
	if(condition){                                          \
		SDL_Log("failed to load media: %s", IMG_GetError());\
		code=EXIT_FAIL;                                     \
	}

typedef struct {
	int x;
	int y;
	int zoom;
} Camera;

typedef struct {
	float frame_time;
	int total_frames;
	int current_frame; /* useless */
	SDL_Rect* frames; /* maybe replace with some math shit */
} Animation;

typedef struct {
	float x, y, speed;
	SDL_Texture* tex;
	Animation* animations;
} ObjectInfo;

enum EXIT_CODE {
	EXIT_FAIL,
	EXIT_OK
};

static char* window_title = "sli";
static const int SWIDTH = 1280; /* screen width */
static const int SHEIGHT = 720; /* screen height */
static const int TARGET_FPS = 1000 / 60; /* second per 60 frames */
static const int GAME_SCALE = 10; /* multiply every render' scale by 5 */

/* temporary level stuff */
static const int CAM_WIDTH = 1280;
static const int CAM_HEIGHT = 720;
static int xcam = 0;
static int ycam = 0;

static SDL_Renderer* renderer = NULL;
static SDL_Window* window = NULL;
static SDL_Surface* icon = NULL;

/* textures */
static SDL_Texture* sli_atlas = NULL;
static SDL_Texture* bg_test = NULL;

/* ObjectInfo */
static ObjectInfo Sli;
static Animation sli_idle;
static Animation sli_walk;
static Animation sli_jump;

/* functions */
static int init(void);
static SDL_Texture* load_texture(const char* path);

static int load_media(void);

/* object related functions */
static Animation create_animation(int total_frames, int atlas_row, int side_len, float frame_time);
static ObjectInfo create_object(char* atlas_path, Animation* anim_set);
static void load_objects(void);
static int update_object(ObjectInfo* oi);
static int render_object(ObjectInfo* oi, int anim_index, int inverted);
static void deinit(void);

/* yes init func */
int
init(void){
	Uint32 sdl_flags = SDL_INIT_VIDEO;
	Uint32 img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
	Uint32 render_flags = SDL_RENDERER_ACCELERATED;

	/* sdl subsystems */
	if(SDL_Init(sdl_flags) != 0) goto FAIL_INIT;
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

	return EXIT_OK; /* success */

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

	return EXIT_FAIL;
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

	bg_test = load_texture("res/test/tmp-bg.png");
	tex_load_failed(!bg_test);

	return code;
}

Animation
create_animation(int total_frames, int atlas_row, int side_len, float frame_time){
	/* total_frames: total frames in the row */
	/* atlas_row:    which row of the atlas to create the animation from(not index 0 based) */
	/* side_len:     side of the square of the sprite */
	/* frame_time:   amount of time the frame will be shown */
	SDL_Rect* rec_arr = (SDL_Rect*)malloc(sizeof(SDL_Rect) * total_frames);

	for(int i = 0; i < total_frames; i++){
		rec_arr[i].x = side_len * i;
		rec_arr[i].y = side_len * (atlas_row - 1);
		rec_arr[i].w = side_len;
		rec_arr[i].h = side_len;
	}

	Animation anim;
	anim.frame_time = frame_time;
	anim.total_frames = total_frames;
	anim.current_frame = 0;
	anim.frames = rec_arr;

	return anim;
}

ObjectInfo
create_object(char* atlas_path, Animation* anim_set){
	ObjectInfo oi;
	oi.tex = load_texture(atlas_path);
	oi.x = 0;
	oi.y = 0;
	oi.speed = 0;
	oi.animations = anim_set;
	return oi;
}

void
load_objects(void){

	/* Sli */
	sli_idle = create_animation(2, 1, 16, 30);
	sli_jump = create_animation(7, 3, 16, 10);
	sli_walk = create_animation(4, 2, 16, 10);

	Animation* set = (Animation*)malloc(sizeof(Animation)*3);
	set[0] = sli_idle;
	set[1] = sli_jump;
	set[2] = sli_walk;

	Sli = create_object("res/sli-atlas.png", set);
	Sli.speed = 1;
	Sli.y = 46;
}

int
render_object(ObjectInfo *oi, int anim_index, int inverted){
	int code = EXIT_OK;

	static int last_anim = 0;
	static int current_frame_time = 0;
	static int curr_frame = 0;

	if(last_anim != anim_index) curr_frame = 0;
	last_anim = anim_index;

	if(current_frame_time >= oi->animations[anim_index].frame_time) {
		current_frame_time = 0;
		curr_frame++;
	}

	if(curr_frame > oi->animations[anim_index].total_frames -1) {
		curr_frame = 0;
	}

	SDL_Rect dst = {
		.x = (oi->x - xcam) * GAME_SCALE,
		.y = (oi->y - ycam) * GAME_SCALE,
		.w = oi->animations[anim_index].frames[curr_frame].w * GAME_SCALE,
		.h = oi->animations[anim_index].frames[curr_frame].h * GAME_SCALE
	};

	// actual rendering
	if(inverted)
		SDL_RenderCopyEx(renderer, oi->tex, &(oi->animations[anim_index].frames[curr_frame]), &dst, 0, NULL, SDL_FLIP_HORIZONTAL);
	else
		SDL_RenderCopy(renderer, oi->tex, &(oi->animations[anim_index].frames[curr_frame]), &dst);
	current_frame_time++;
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
	int inverted = 0;
	float grav = 0.08f;
	float vsp = 0.0f;
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

			if(e.type == SDL_KEYDOWN)
				switch(e.key.keysym.sym) {
					case SDLK_ESCAPE:
						quit = 1;
						break;
					// case SDLK_RIGHT:
					// 	Sli.x += 1;
					// 	break;
				}
		}

		int toRender = 0;
		const Uint8* keyState = SDL_GetKeyboardState(NULL);
		int mov = keyState[SDL_SCANCODE_RIGHT] - keyState[SDL_SCANCODE_LEFT];
		int jmp = keyState[SDL_SCANCODE_SPACE];
		int hsp = Sli.speed * mov;

		if(hsp > 0) {
			toRender = 2;
			inverted = 0;
		} else if(hsp < 0) {
			toRender = 2;
			inverted = 1;
		}

		if(Sli.y + 4 <= 46) {
			toRender = 1;
			vsp += grav;
		} else {
			vsp = 0;
			if(jmp) {
				vsp = -2.0f;
			}
		}

		Sli.x += hsp;
		Sli.y += vsp;

		/* create camera */
		SDL_Rect camera = {
			.x = (Sli.x - 16) - ((CAM_WIDTH/10) / 2),
			.y = (Sli.y + 16) - ((CAM_HEIGHT/10) / 2),
			.w = CAM_WIDTH/10,
			.h = CAM_HEIGHT/10
		};

		/* keep the camera in bounds */
		if(camera.x < 0) camera.x = 0;
		if(camera.y < 0) camera.y = 0;
		if(camera.x + camera.w > 256) camera.x = 256 - camera.w;
		if(camera.y + camera.h > 72) camera.y = 72 - camera.h;

		xcam = camera.x;
		ycam = camera.y;
		
		/* render onto the framebuffer */
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, bg_test, &camera, NULL);

		/* render nice objects */
		render_object(&Sli, toRender, inverted);

		/* flip framebuffer */
		SDL_RenderPresent(renderer);

		/* delta frame (well... yes, but actually, no) */
		delta = SDL_GetTicks() - delta;
		if(delta < TARGET_FPS)
			SDL_Delay(TARGET_FPS - delta);
		frame_count++;
		// SDL_Log("frames: %d", frame_count);
	}

FAIL_LOAD_OBJECTS:
FAIL_LOAD_MEDIA:
FAIL_MAIN:
	deinit();
	return 0;
}
