#include "main.h"

SDL_GLContext gContext;
SDL_Window* mywindow = NULL;			

int FSAA = 1;
int FULLSCREEN = 1;
int WINDOW_WIDTH = 1280;
int WINDOW_HEIGHT = 720;
screen_t screen;
extern particle_t *parts;
extern explosion_t *explode;
extern int MAX_PARTICLES;
extern int no_of_asteroids;
extern Camera camera;
extern GLuint tex;
extern GLuint powertex;
TTF_Font *score_font = NULL;
TTF_Font *title_font = NULL;
TTF_Font *menu_font = NULL;
GLfloat *part_colors = NULL;
GLfloat *part_vertices = NULL;
GLfloat asteroid_offsets[ASTEROID_VERTS] = {
													0, 		0, 
													0, 		-4.5, 
													3, 		-3.5, 
													3, 		-2, 
													4, 		1, 
													2, 		4.5, 
													-2, 	4.5, 
													-4.5, 	1, 
													-3, 	-2, 
													-2.5, 	-3, 
													0, 		-4.5};
void layer_save_as_ppm(uint8_t *pixels, int width, int height, const char *file_path);
static void update_screen_metrics(void);
static Uint32 window_flags(void);
static void window_size(int *w, int *h);
static void reload_fonts(void);
static float world_scale_from_window(void);

int init_sdl(){

	if (SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("init error! sdl: %s\n", SDL_GetError());
		return 1;
	}

	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1);
	if (FSAA){
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
	}
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	int w = 0;
	int h = 0;
	window_size(&w, &h);
	Uint32 flags = window_flags();
	mywindow = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flags);
	if (mywindow == NULL){
		printf("error creating window! sdl:%s\n", SDL_GetError());
		return 1;
	}

	gContext = SDL_GL_CreateContext(mywindow);
	SDL_GL_SetSwapInterval(1);
	update_screen_metrics();

	flags = IMG_INIT_PNG;

	if (!(IMG_Init(flags) & flags)){
		printf("png init error! sdl: %s\n", SDL_GetError());
		return 1;
	}

	if (TTF_Init() == -1){
		printf( "SDL_ttf could not initialize! SDL_ttf Error: %s\n", TTF_GetError() );
		return 1;
	}
	glewExperimental = GL_TRUE;
	glewInit();
	initGL();
	reload_fonts();
	SDL_SetRelativeMouseMode(1);
	return 0;
}

void reload_context(){

	SDL_GL_DeleteContext(gContext);
	gContext = NULL;
	SDL_DestroyWindow(mywindow);
	mywindow = NULL;
	
	int w = 0;
	int h = 0;
	window_size(&w, &h);
	mywindow = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, window_flags());
	if (mywindow == NULL){
		printf("error creating window! sdl:%s\n", SDL_GetError());
	}
	gContext = SDL_GL_CreateContext(mywindow);
	SDL_GL_SetSwapInterval(1);
	update_screen_metrics();
	reload_fonts();
	SDL_SetRelativeMouseMode(1);

	tex = load_texture("./data/stars.png");
	powertex = load_texture("./data/power.png");
}

static void update_screen_metrics(void){

	int w = 0;
	int h = 0;
	SDL_GetWindowSize(mywindow, &w, &h);
	if (w <= 0 || h <= 0)
		SDL_GL_GetDrawableSize(mywindow, &w, &h);
	screen.width = (float)w;
	screen.height = (float)h;

	if (SDL_GetDisplayDPI(0, &screen.d_dpi, &screen.h_dpi, &screen.v_dpi)){
		SDL_Log("ERROR: unable to read dpi\n");
		screen.d_dpi = 96.0f;
		screen.h_dpi = 96.0f;
		screen.v_dpi = 96.0f;
	}
	screen.obj_scale_factor = screen.d_dpi / 80.0f;
	screen.ctl_scale_factor = screen.obj_scale_factor * 0.05f;
}

static void reload_fonts(void){

	if (score_font){
		TTF_CloseFont(score_font);
		score_font = NULL;
	}
	if (title_font){
		TTF_CloseFont(title_font);
		title_font = NULL;
	}
	if (menu_font){
		TTF_CloseFont(menu_font);
		menu_font = NULL;
	}

	score_font = TTF_OpenFont("./data/HyperspaceBold.ttf", (int)(screen.height * 0.025f));
	if (score_font == NULL) exit(0);
	title_font = TTF_OpenFont("./data/spaceranger.ttf", (int)(screen.height * 0.14f));
	if (title_font == NULL) exit(0);
	menu_font = TTF_OpenFont("./data/HyperspaceBold.ttf", (int)(screen.height * 0.08f));
	if (menu_font == NULL) exit(0);
}

static float world_scale_from_window(void){

	float min_dim = screen.width < screen.height ? screen.width : screen.height;
	float scale = min_dim / 1080.0f;
	if (scale < 0.5f) scale = 0.5f;
	if (scale > 2.0f) scale = 2.0f;
	return scale;
}

static Uint32 window_flags(void){

	Uint32 flags = SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_OPENGL;
	if (FULLSCREEN){
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	}
	else{
		flags |= SDL_WINDOW_SHOWN;
	}
	return flags;
}

static void window_size(int *w, int *h){

	SDL_DisplayMode dm;
	SDL_GetDesktopDisplayMode(0, &dm);
	if (FULLSCREEN){
		*w = 0;
		*h = 0;
	}
	else{
		*w = WINDOW_WIDTH;
		*h = WINDOW_HEIGHT;
		if (*w > dm.w) *w = dm.w;
		if (*h > dm.h) *h = dm.h;
		if (*w < 960) *w = 960;
		if (*h < 540) *h = 540;
	}
}

void set_fullscreen(int enabled){

	int want_fullscreen = enabled ? 1 : 0;
	if (want_fullscreen == FULLSCREEN)
		return;

	FULLSCREEN = want_fullscreen;
	if (FULLSCREEN){
		SDL_SetWindowFullscreen(mywindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
	}
	else{
		SDL_SetWindowFullscreen(mywindow, 0);
		int w = 0;
		int h = 0;
		window_size(&w, &h);
		SDL_SetWindowSize(mywindow, w, h);
		SDL_SetWindowPosition(mywindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	}
	update_screen_metrics();
	initGL();
	reload_fonts();
	SDL_SetRelativeMouseMode(1);
}

void set_window_resolution(int width, int height){

	if (width < 960) width = 960;
	if (height < 540) height = 540;
	WINDOW_WIDTH = width;
	WINDOW_HEIGHT = height;

	if (FULLSCREEN)
		return;

	SDL_SetWindowSize(mywindow, WINDOW_WIDTH, WINDOW_HEIGHT);
	SDL_SetWindowPosition(mywindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	update_screen_metrics();
	initGL();
	reload_fonts();
	SDL_SetRelativeMouseMode(1);
}

void toggle_anti_aliasing(int value){

	if (!value){
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);		
		reload_context();	
		initGL();
		glDisable(GL_MULTISAMPLE);
		FSAA = 0; 
	}
	else {
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);		
		reload_context();
		initGL();
		glEnable(GL_MULTISAMPLE);
		FSAA = 1;
	}
}

void initGL(){

	GLenum error = GL_NO_ERROR;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, screen.width, screen.height, 0, -10, 10);

	error = glGetError();
	if (error != GL_NO_ERROR){
		SDL_Log("Error initializing OpenGL! %d\n", error);
	}

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPushMatrix();

	error = glGetError();
	if (error != GL_NO_ERROR){
		SDL_Log( "Error initializing OpenGL! %d\n", error);
	}

	glClearColor(0.f, 0.f, 0.f, 1.f);
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glHint(GL_LINE_SMOOTH_HINT,  GL_NICEST);
	int drawable_w = 0;
	int drawable_h = 0;
	SDL_GL_GetDrawableSize(mywindow, &drawable_w, &drawable_h);
	if (drawable_w <= 0 || drawable_h <= 0){
		drawable_w = (int)screen.width;
		drawable_h = (int)screen.height;
	}
	glViewport(0, 0, drawable_w, drawable_h);
}

void close_sdl(){

	for (int i=0;i<MAX_SOUNDS;++i){
		if (sound[i].effect != NULL){
			Mix_FreeChunk(sound[i].effect);
		}
	}
	shutdown_music_player();
 	Mix_CloseAudio();
	TTF_CloseFont(score_font);
	TTF_CloseFont(title_font);
	TTF_CloseFont(menu_font);
	score_font = NULL;
	SDL_DestroyWindow(mywindow);
	mywindow = NULL;
	free(explode);
	free(parts);
	free(part_colors);
	free(part_vertices);
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

GLuint load_texture(const char *s){

	GLuint newtexture;
	newtexture = 0;
	SDL_Surface* loaded = IMG_Load(s);
	if (loaded == NULL){
		printf("file not here? sdl: %s\n", SDL_GetError());
	}

	else{	
		glGenTextures(1, &newtexture);
		glBindTexture(GL_TEXTURE_2D, newtexture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, loaded->w, loaded->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, loaded->pixels);
		SDL_FreeSurface(loaded);
	}
	return newtexture;
}

int power_two(int n){

	unsigned int x;
	x = n;
	x--;
    x |= x >> 1;  // handle  2 bit numbers
    x |= x >> 2;  // handle  4 bit numbers
    x |= x >> 4;  // handle  8 bit numbers
    x |= x >> 8;  // handle 16 bit numbers
    x |= x >> 16; // handle 32 bit numbers
    x++;
    return (int)x;
}

void render_text(texture_t *t, char s[], int c, int size){

	TTF_Font 		*text_font = menu_font;
	SDL_Color 		col = {53, 128, 35, 255};
	SDL_Color 		col2 = {255, 255, 255, 255};
	SDL_Color 		col3 = {17, 126, 255, 255};
	SDL_Surface* 	load_text = NULL;
	
	// 24 is magic size for scorebar
	if (size == 24){
		load_text = TTF_RenderText_Blended(score_font, s, col2);
	}
	// high scores
	else if (size == 22){

		load_text = TTF_RenderText_Blended(score_font, s, col3);
	}

	else{
		
		if (size == 48)
			text_font = menu_font;
		if (size == 25)
			text_font = score_font;
		if (size == 88)
			text_font = title_font;
			
		if (c == 0)
			load_text = TTF_RenderText_Blended(text_font, s, col);
		else if (c == 1)
			load_text = TTF_RenderText_Blended(text_font, s, col2);
		else
			load_text = TTF_RenderText_Blended(text_font, s, col3);
	}

	// SDL does not always pack the bytes in its pixel structures tightly
	// for some versions of opengl we can use glpixelstore command to tell
	// opengl about this otherwise we must repack the pixels in th buffer.
	// Uint32 len = surface->w * surface->format->BytesPerPixel;
	// Uint8 *src = surface->pixels;
	// Uint8 *dst = surface->pixels;
	// for (i = 0; i < surface->h; i++) {
    	// SDL_memmove(dst, src, len);
    	// dst += len; 
    	// src += surface->pitch;
	// }    
	// surface->pitch = len;
	int pitch = load_text->pitch;
	int p2x;
	int p2y;
	p2x = power_two(load_text->w);
	p2y = power_two(load_text->h);
	
	SDL_Surface* resized = SDL_CreateRGBSurface(0, p2x, p2y, 32, 0, 0, 0, 0);
	//if (resized == NULL) exit(0);
	t->w = load_text->w;
	t->h = load_text->h;
	int offsetx = (p2x - t->w)/2 - 1;
	int offsety = (p2y - t->h)/2 - 1;
	
		
	glGenTextures(1, &t->t);
	glBindTexture(GL_TEXTURE_2D, t->t);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, p2x, p2y, 0, GL_RGBA, GL_UNSIGNED_BYTE, resized->pixels);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
	glTexSubImage2D(GL_TEXTURE_2D, 0, offsetx, offsety, t->w, t->h, GL_RGBA, GL_UNSIGNED_BYTE, load_text->pixels);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	t->w = p2x;
	t->h = p2y;
	SDL_FreeSurface(load_text);
	SDL_FreeSurface(resized);
	load_text = NULL;
	resized = NULL;
	text_font = NULL;

	return;
}

void layer_save_as_ppm(uint8_t *pixels, int width, int height, const char *file_path)
{
    FILE *f = fopen(file_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "ERROR: could not open file %s: %m\n",
                file_path);
        exit(1);
    }

    fprintf(f, "P6\n%d %d 255\n", width, height);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            
            char pixel[3];
            // skip the alpha channel
            ++pixels;
            pixel[0] = *pixels;
            ++pixels;
            pixel[1] = *pixels;
            ++pixels;
            pixel[2] = *pixels;

            fwrite(pixel, sizeof(pixel), 1, f);
        }
        
    }

    fclose(f);
}

void draw_background(GLuint stars){

	update_music_player();

	float x = camera.x/screen.width;
	float y = camera.y/screen.height;
	
	glMatrixMode(GL_MODELVIEW);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, stars);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	GLfloat tex[] = {	0.0f - x, 1.0f - y,
						0.0f - x, 0.0f - y,
						1.0f - x, 1.0f - y,
						1.0f - x, 0.0f - y };

	GLfloat box[] = {0, screen.height, 0, 0, screen.width, screen.height, screen.width, 0};
	glColor4f(0.5, 1.0, 1.0, 1.0);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, box);
	glTexCoordPointer(2, GL_FLOAT, 0, tex);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void draw_scorebar(Player_t *p, int mode){

	static texture_t t1;
	static texture_t t2;
	static int old_score;
	static int old_lives;
	static int old_shield;
	static int old_score2;
	static int old_lives2;
	static int old_shield2;
	char str[64];
	char str2[64];
	
	GLfloat tex[] = {0, 1, 0, 0, 1, 1, 1, 0};
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	if (mode == 0){
		if (p[0].score != old_score || p[0].lives != old_lives || (int)p[0].sh.shield_power != old_shield){
			
			glDeleteTextures(1, &t1.t);
			
			sprintf(str, "SCORE: %05d    LIVES: %d    SHIELD: %2d", p[0].score, p[0].lives, (int)p[0].sh.shield_power);
			render_text(&t1, str, 1, 24);
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, t1.t);

		GLfloat box[] = {screen.width/2 - t1.w/2, t1.h,
						 screen.width/2 - t1.w/2, 0,
						 screen.width/2 + t1.w/2, t1.h,
						 screen.width/2 + t1.w/2, 0};
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	 
		glVertexPointer(2, GL_FLOAT, 0,box);
		glTexCoordPointer(2, GL_FLOAT, 0, tex);
		glColor4f( 1.0, 1.0, 0.5, 1.0 );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	 
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
		
		old_score = p[0].score;
		old_lives = p[0].lives;
		old_shield = (int)p[0].sh.shield_power;
	}

	else {
		if (p[0].score != old_score || p[0].lives != old_lives || (int)p[0].sh.shield_power != old_shield){			
			glDeleteTextures(1, &t1.t);			
			sprintf(str, "SCORE: %05d    LIVES: %d    SHIELD: %2d", p[0].score, p[0].lives, (int)p[0].sh.shield_power);
			render_text(&t1, str, 1, 24);
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, t1.t);
		GLfloat box[] = {10, t1.h, 10, 0, t1.w + 10, t1.h, t1.w + 10, 0};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	 
		glVertexPointer(2, GL_FLOAT, 0,box);
		glTexCoordPointer(2, GL_FLOAT, 0, tex);
		glColor4f( 1.0, 1.0, 0.5, 1.0 );
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	 
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);
		
		old_score = p[0].score;
		old_lives = p[0].lives;
		old_shield = (int)p[0].sh.shield_power;
		
		//
		if (p[1].score != old_score2 || p[1].lives != old_lives2 || (int)p[1].sh.shield_power != old_shield2){

			glDeleteTextures(1, &t2.t);
			sprintf(str2, "SCORE: %05d    LIVES: %d    SHIELD: %2d", p[1].score, p[1].lives, (int)p[1].sh.shield_power);
			render_text(&t2, str2, 1, 24);
		}

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, t2.t);
		GLfloat box2[] = {screen.width - 10 - t2.w, t2.h,
						  screen.width - 10 - t2.w, 0,
						  screen.width -10, t2.h,
						  screen.width -10, 0};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	 
		glVertexPointer(2, GL_FLOAT, 0, box2);
		glTexCoordPointer(2, GL_FLOAT, 0, tex);
		glColor4f(0.5, 1.0, 1.0, 1.0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	 
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisable(GL_TEXTURE_2D);

		old_score2 = p[1].score;
		old_lives2 = p[1].lives;
		old_shield2 = (int)p[1].sh.shield_power;
	}
}

void draw_asteroids(asteroid_t *field){

	static const GLfloat base_colors[44] = {
		0.9f, 0.3f, 0.6f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f,
		0.5f, 0.3f, 0.0f, 1.0f
	};
	GLfloat colors[44];
	const float t = SDL_GetTicks() * 0.0015f;
	
	for (int i = 0; i < no_of_asteroids; i++){
		float world_scale = world_scale_from_window();
		float base_size = field[i].size / world_scale;
		if (base_size < 3.0f) base_size = 3.0f;
		if (base_size > 12.0f) base_size = 12.0f;
		float size_t = (base_size - 3.0f) / 9.0f;           // 0 small -> 1 large
		float size_brightness = 1.18f - 0.23f * size_t;     // small asteroids brighter
		// Subtle, slow pulse with per-asteroid phase offset.
		float pulse = 0.88f + 0.12f * sinf(t + i * 0.7f);
		float brightness = pulse * size_brightness;
		for (int c = 0; c < 44; c += 4){
			colors[c + 0] = fminf(1.0f, base_colors[c + 0] * brightness);
			colors[c + 1] = fminf(1.0f, base_colors[c + 1] * brightness);
			colors[c + 2] = fminf(1.0f, base_colors[c + 2] * brightness);
			colors[c + 3] = base_colors[c + 3];
		}
		
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_COLOR_ARRAY);
		
		glVertexPointer(2, GL_FLOAT, 0, asteroid_offsets);
		glColorPointer(4, GL_FLOAT, 0, colors);
		glPushMatrix();
		
		glTranslatef(field[i].position.x, field[i].position.y, 0);
		glRotatef(field[i].angle * 180/PI, 0, 0, 1);
		glScalef(field[i].size, field[i].size, 0);
	
		glDrawArrays(GL_TRIANGLE_FAN, 0, 11);
		glPopMatrix();

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}
}

void draw_ship(ship_t *sh, int type){

	static float ship_offsets[12] = {0, 0, 12, 0, -8, 8, -4, 0, -8, -8, 12, 0};
	static float ship_offsets2[12] = {0, 0, 14, -8, -8, -8,-4, 0, -8, 8, 14, 8};
	float *offsets;
	
	if (type == 0){
		glColor4f(0.9f, 0.6f, 0.9f, 0.9f);
		offsets = ship_offsets;
	}
	if (type == 1){
		glColor4f(0.5f, 0.7f, 0.9f, 0.9f);
		offsets = ship_offsets;
	}
	if (type == 2){
		glColor4f(0.5f, 0.9f, 0.3f, 0.9f);
		offsets = ship_offsets2;
	}
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, offsets);
	glPushMatrix();
		float ship_scale = world_scale_from_window();
		glTranslatef(sh->position.x, sh->position.y, 0);
		glRotatef(sh->angle * 180/PI, 0, 0, 1);
		glScalef(ship_scale, ship_scale, 0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
	glPopMatrix();
	glDisableClientState(GL_VERTEX_ARRAY);

	// DRAW THRUST
	GLfloat colors[4*12];
	GLfloat vertices[2*12];
	int ci = 0;
	int vi = 0;

	for (int i= 0; i < 12; ++i){
		if (sh->thrust[i].state == 1){	
			colors[ci++] = 1.8* sh->thrust[i].age;
			colors[ci++] = 1.05* sh->thrust[i].age;
			colors[ci++] = 1.5* sh->thrust[i].age;
			colors[ci++] = 1 * sh->thrust[i].age;
			vertices[vi++] = sh->thrust[i].position.x;
			vertices[vi++] = sh->thrust[i].position.y;
		}
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glColorPointer(4, GL_FLOAT, 0, colors);
	glDrawArrays(GL_POINTS, 0, vi/2);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

void draw_shield(ship_t *sh){

	static float ship_shield[68] = {0, 0, 16, 0, 15, 3, 14, 6, 13, 9, 11, 11, 8, 13, 5, 14, 2, 15, 0, 15,
	-3, 15, -6, 14, -9, 12, -11, 10, -13, 8, -15, 5, -15, 2, -15, 0,
	-15, -4, -14, -7, -12, -9, -10, -12, -7, -13, -4, -15, -1, -15,
	1, -15, 4, -15, 7, -14, 10, -12, 12, -10, 14, -7, 15, -4, 15, -1, 16, 0};

		
	glColor4ub(237, 245, 50, 70);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, ship_shield);
	glPushMatrix();
		float ship_scale = world_scale_from_window();
		glTranslatef(sh->position.x, sh->position.y, 0);
		glScalef(ship_scale, ship_scale, 0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 34);
	glPopMatrix();
	glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_enemy_shield(ship_t *sh){

	static float ship_shield[68] = {0, 0, 16, 0, 15, 3, 14, 6, 13, 9, 11, 11, 8, 13, 5, 14, 2, 15, 0, 15,
	-3, 15, -6, 14, -9, 12, -11, 10, -13, 8, -15, 5, -15, 2, -15, 0,
	-15, -4, -14, -7, -12, -9, -10, -12, -7, -13, -4, -15, -1, -15,
	1, -15, 4, -15, 7, -14, 10, -12, 12, -10, 14, -7, 15, -4, 15, -1, 16, 0};

	glColor4ub(80, 255, 130, 78);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, ship_shield);
	glPushMatrix();
		float ship_scale = world_scale_from_window();
		glTranslatef(sh->position.x, sh->position.y, 0);
		glScalef(ship_scale, ship_scale, 0);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 34);
	glPopMatrix();
	glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_bullets(bullet_t *bulls){

	GLfloat vertices[2*MAX_BULLETS];
	glColor4f(0.9f, 0.7f, 0.7f, 1);
	glPointSize(3.0f * screen.obj_scale_factor);
	int vi = 0;
	
	for (int i = 0; i < MAX_BULLETS; ++i){
		if (bulls[i].position.x != -99999){
			vertices[vi++] = bulls[i].position.x;
			vertices[vi++] = bulls[i].position.y;
		}
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, vertices);
	glDrawArrays(GL_POINTS, 0, vi/2);
	glDisableClientState(GL_VERTEX_ARRAY);
}

void draw_particles(){
	
	int ci = 0;		// index for colour array 
	int vi = 0;		// index for vertex array

	for (int i = 0; i < MAX_PARTICLES; ++i){
		if (parts[i].state == 1){
			part_colors[ci] = 1.2 * parts[i].age;		// calculate colours of each living particle and put in array
			part_colors[ci+1] = 0.7 * parts[i].age;
			part_colors[ci+2] = 0.6 * parts[i].age;
			part_colors[ci+3] = 0.5 * parts[i].age;
			ci = ci + 4;

			part_vertices[vi] = parts[i].position.x;		// insert particles position to vertex array
			part_vertices[vi+1] = parts[i].position.y;
			vi = vi + 2;
		}	
	}
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glPointSize(2.0f * screen.obj_scale_factor);
	glVertexPointer(2, GL_FLOAT, 0, part_vertices);
	glColorPointer(4, GL_FLOAT, 0, part_colors);
	glDrawArrays(GL_POINTS, 0, vi/2);

	glPointSize(5.0f * screen.obj_scale_factor);
	glVertexPointer(2, GL_FLOAT, 0, part_vertices);
	glColorPointer(4, GL_FLOAT, 0, part_colors);
	glDrawArrays(GL_POINTS, 0, vi/2);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glPointSize(1.0f * screen.obj_scale_factor);
}

void draw_explosion(explosion_t *e){

	static const GLfloat fan[68] = {
		0, 0, 16, 0, 15, 3, 14, 6, 13, 9, 11, 11, 8, 13, 5, 14, 2, 15, 0, 15,
		-3, 15, -6, 14, -9, 12, -11, 10, -13, 8, -15, 5, -15, 2, -15, 0,
		-15, -4, -14, -7, -12, -9, -10, -12, -7, -13, -4, -15, -1, -15,
		1, -15, 4, -15, 7, -14, 10, -12, 12, -10, 14, -7, 15, -4, 15, -1, 16, 0};
	static const GLfloat colors1[272] =
		{1, 0.78, 0.7, 0.9, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4,
		1, 0.78, 0.7, 0.3, 1, 0.7, 0.2, 0.4};
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	glBlendFunc(GL_ONE, GL_ONE);
	glVertexPointer(2, GL_FLOAT, 0, fan);
	glColorPointer(4, GL_FLOAT, 0, colors1);
	glPushMatrix();

	glTranslatef(e->x, e->y, 0);
	glRotatef(e->age * 6, 0, 0, 1);
	if (e->age > 2)
		glScalef(4 - e->age, 4 - e->age, 0);
	if (e->age < 2)
		glScalef(0.1 + e->age, 0.1 + e->age, 0);
	
	glDrawArrays(GL_TRIANGLE_FAN, 0, 34);

	glPopMatrix();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);	
}

void draw_powerup(powerup_t *p) {

	p->age = ((p->age + 2) % 200);
	glColor4ub(255, 255, 255, p->age + 55);

	GLfloat w_offset;
	GLfloat h_offset;
	GLfloat width = 0.5;
	switch (p->type) {
	case 0:
		w_offset = 0.0f;
		h_offset = 1.0f;
		break;
	case 1: // lives
		w_offset = 0.5f;
		h_offset = 0.5f;
		break;
	case 2:
		w_offset = 0.0f;
		h_offset = 0.5f;
		break;
	case 3: // d bullets
		w_offset = 0.5f;
		h_offset = 1.0f;
		break;
	}

	GLfloat ptex[] = { 	w_offset, h_offset + 0.5f,
				w_offset, h_offset,
				w_offset + width, h_offset + 0.5f,
				w_offset + width, h_offset };
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, powertex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	
	GLfloat box[] = { 	-15,  15,
						-15, -15,
						 15,  15,
						 15, -15};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(2, GL_FLOAT, 0, box);
	glTexCoordPointer(2, GL_FLOAT, 0, ptex);
	glPushMatrix();
		glTranslatef(p->position.x, p->position.y, 0);
		glScalef(screen.obj_scale_factor, screen.obj_scale_factor, 0);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glPopMatrix();
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void level_transition(int level){

	char text[12];
	int quit = 0;
	int startTicks;
	int timer = 0;
	SDL_Event e;
	play_sound(ENTRY, 0);
	startTicks = SDL_GetTicks(); 
	sprintf(text, "level %d", level);
			
	while (!quit){
		
		glClear (GL_COLOR_BUFFER_BIT);
		glColor4f(1,1,1,1);
		draw_menu_item(screen.width/2, screen.height/2, text, 2, 88);
		while (SDL_PollEvent(&e) != 0){		// check for key event

			timer = SDL_GetTicks() - startTicks;

			if (timer < 500)
				break;
			if(e.type == SDL_KEYDOWN || (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT))){
				quit = 1;
			}
		}
		
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);		
	}
}
