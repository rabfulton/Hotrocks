// Hot Rocks
#define APPNAME "Hotrocks"

#include "main.h"
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>
Sound sound[MAX_SOUNDS];				// Array for sounds index				
int layout[11];							// Array for current keymap
int run_state;
int SFX_VOLUME = MIX_MAX_VOLUME;
int MUSIC_VOLUME = MIX_MAX_VOLUME;
extern GLfloat asteroid_offsets[ASTEROID_VERTS];		// 22 verts tri fan
Vector min_seperate(Vector p1, int s1, Vector p2, int s2);
void move_ship2(ship_t *sh, asteroid_t *field, bullet_t *bulls, enemy_t *en);
void collide2(Vector *v1, Vector *p1, int s1, Vector *v2, Vector *p2, int s2);
int keyboard_events(Player_t *p1, Player_t *p2, int mode);
int pause_game();
void player_wins(int n);
void init_powerup(float x, float y);
void collect_powerup(enemy_t *en, Player_t *p, int i);
static int check_for_collision(int x, int y);
static inline int test_triangle_point(Vector a, Vector b, Vector c, Vector p);
float v_dot(Vector a, Vector b);
Vector v_sub(Vector a, Vector b);

extern GLfloat *part_colors;
extern GLfloat *part_colors_inner;
extern GLfloat *part_vertices;
//extern GLfloat *asteroid_offsets;
extern screen_t screen;
extern SDL_Window *mywindow;
int dual_bullets;
int no_of_enemy = 0;
int no_of_asteroids = 0;
int no_of_powerups = 0;
static int screenshot_requested = 0;
int NO_OF_EXPLOSIONS = 32;		// Must be divisor of MAX_PARTICLES
int MAX_PARTICLES = 2048;		// Must be multiple of NO_OF_EXPLOSIONS
int render = 1;
float time_delta = 0;			// the time elapsed since last frame was rendered
GLuint tex;
GLuint powertex;	
Camera camera;
explosion_t *explode = NULL;		// pointer to array sized to Number of simultaneous explosions
particle_t *parts = NULL;			// pointer to array for particle engine
powerup_t powerups[MAX_POWERUPS];
asteroid_t field[MAX_ASTEROIDS];
enemy_t en[MAX_ENEMY];
	
int cam = 1; 					// 0 = fixed, 1 = lag, 2 = traditional
static openmpt_module *music_module = NULL;
static uint8_t *music_file_data = NULL;
static size_t music_file_size = 0;
static int music_sample_rate = 44100;
static int music_channels = 2;
static int music_enabled = 0;
static SDL_atomic_t music_track_finished;

static float window_world_scale(void){

	float min_dim = screen.width < screen.height ? screen.width : screen.height;
	float scale = min_dim / 1080.0f;
	if (scale < 0.5f) scale = 0.5f;
	if (scale > 2.0f) scale = 2.0f;
	return scale;
}

static int is_module_extension(const char *name){

	const char *ext = strrchr(name, '.');
	if (!ext || *(ext + 1) == '\0')
		return 0;
	++ext;

	return !SDL_strcasecmp(ext, "mod") ||
		   !SDL_strcasecmp(ext, "xm") ||
		   !SDL_strcasecmp(ext, "s3m") ||
		   !SDL_strcasecmp(ext, "it") ||
		   !SDL_strcasecmp(ext, "mptm") ||
		   !SDL_strcasecmp(ext, "symmod");
}

static void free_mod_list(char **mods, int count){

	if (!mods)
		return;

	for (int i = 0; i < count; ++i)
		free(mods[i]);
	free(mods);
}

static int scan_mod_directory(char ***mods_out, int *count_out){

	DIR *dir = opendir("./mods");
	if (!dir){
		SDL_Log("mods directory unavailable: %s", strerror(errno));
		return 0;
	}

	char **mods = NULL;
	int count = 0;
	struct dirent *entry = NULL;
	while ((entry = readdir(dir)) != NULL){
		if (entry->d_name[0] == '.')
			continue;
		if (!is_module_extension(entry->d_name))
			continue;

		char path[512];
		snprintf(path, sizeof(path), "./mods/%s", entry->d_name);
		SDL_RWops *f = SDL_RWFromFile(path, "rb");
		if (!f)
			continue;
		SDL_RWclose(f);

		char **next = realloc(mods, (size_t)(count + 1) * sizeof(char *));
		if (!next){
			free_mod_list(mods, count);
			closedir(dir);
			return 0;
		}
		mods = next;
		mods[count] = SDL_strdup(path);
		if (!mods[count]){
			free_mod_list(mods, count);
			closedir(dir);
			return 0;
		}
		++count;
	}
	closedir(dir);

	*mods_out = mods;
	*count_out = count;
	return 1;
}

static void clear_current_music(void){

	SDL_LockAudio();
	openmpt_module *old_module = music_module;
	music_module = NULL;
	SDL_UnlockAudio();

	if (old_module)
		openmpt_module_destroy(old_module);

	free(music_file_data);
	music_file_data = NULL;
	music_file_size = 0;
}

static int load_random_module(void){

	char **mods = NULL;
	int mod_count = 0;
	if (!scan_mod_directory(&mods, &mod_count) || mod_count == 0){
		free_mod_list(mods, mod_count);
		clear_current_music();
		return 0;
	}

	int index = rand() % mod_count;
	const char *path = mods[index];
	SDL_RWops *file = SDL_RWFromFile(path, "rb");
	if (!file){
		free_mod_list(mods, mod_count);
		return 0;
	}

	Sint64 size = SDL_RWsize(file);
	if (size <= 0 || size > 64 * 1024 * 1024){
		SDL_RWclose(file);
		free_mod_list(mods, mod_count);
		return 0;
	}

	uint8_t *data = malloc((size_t)size);
	if (!data){
		SDL_RWclose(file);
		free_mod_list(mods, mod_count);
		return 0;
	}

	const size_t got = SDL_RWread(file, data, 1, (size_t)size);
	SDL_RWclose(file);
	if (got != (size_t)size){
		free(data);
		free_mod_list(mods, mod_count);
		return 0;
	}

	int error = OPENMPT_ERROR_OK;
	const char *error_message = NULL;
	openmpt_module *next = openmpt_module_create_from_memory2(
		data,
		(size_t)size,
		NULL,
		NULL,
		NULL,
		NULL,
		&error,
		&error_message,
		NULL
	);
	if (!next){
		SDL_Log("Failed to load module %s: %s", path, error_message ? error_message : "unknown libopenmpt error");
		free(data);
		free_mod_list(mods, mod_count);
		return 0;
	}

	clear_current_music();
	music_file_data = data;
	music_file_size = (size_t)size;
	SDL_LockAudio();
	music_module = next;
	SDL_UnlockAudio();
	SDL_AtomicSet(&music_track_finished, 0);

	SDL_Log("Now playing module: %s", path);
	free_mod_list(mods, mod_count);
	return 1;
}

static void music_hook(void *udata, uint8_t *stream, int len){

	(void)udata;
	memset(stream, 0, (size_t)len);
	if (!music_enabled || !music_module || len <= 0 || music_channels != 2)
		return;

	const size_t frames = (size_t)len / (sizeof(int16_t) * 2);
	if (frames == 0)
		return;

	int16_t *out = (int16_t *)stream;
	size_t rendered = openmpt_module_read_interleaved_stereo(music_module, music_sample_rate, frames, out);
	if (rendered < frames){
		memset(out + rendered * 2, 0, (frames - rendered) * sizeof(int16_t) * 2);
		SDL_AtomicSet(&music_track_finished, 1);
	}

	if (MUSIC_VOLUME < MIX_MAX_VOLUME){
		for (size_t i = 0; i < rendered * 2; ++i){
			int sample = out[i];
			sample = (sample * MUSIC_VOLUME) / MIX_MAX_VOLUME;
			if (sample > INT16_MAX) sample = INT16_MAX;
			if (sample < INT16_MIN) sample = INT16_MIN;
			out[i] = (int16_t)sample;
		}
	}
}

int init_game(){

	load_keys(0);

	if (init_sdl() != 0){
		SDL_Log("initialisation error\n");
		close_sdl();
		return 1;
	}

	init_sound();
	tex = load_texture("./data/stars.png");
	powertex = load_texture("./data/power.png");	
	allocate_memory();
	return 0;
}

void allocate_memory(){
	
	explode = realloc(explode, NO_OF_EXPLOSIONS * sizeof(explosion_t));	
	parts = realloc(parts, MAX_PARTICLES * sizeof(particle_t));
	part_colors = realloc(part_colors, 4 * MAX_PARTICLES * sizeof(float));
	part_colors_inner = realloc(part_colors_inner, 4 * MAX_PARTICLES * sizeof(float));
	part_vertices = realloc(part_vertices, 2 * MAX_PARTICLES * sizeof(float));
}

int main(int argc, char *argv[]){

	srand(time(NULL));	
	
	int mode = 0;
	Player_t p[2];
	Player_t *p1, *p2;
	p1 = &p[0];
	p2 = &p[1];
	
	if (init_game()){
		goto end;
	}
	set_game_mode:
	mode = main_menu(tex);
	if (mode == 99)
		goto end;

	
	
	int quit = 0;
	int status = 0;
	int level = 1;
	int endtimer = 0;				// end of level or respawn timer
	int endtimer_flag = 0;			// end timer running flag
	
	init_level(&field[0], &p[0], &en[0], level, mode);
	init_particles();

	Uint32 startclock = 0;
	Uint32 deltaclock = 0;
	Uint32 oldclock = SDL_GetTicks();
	Uint32 time_delta = 0;

	while (!quit){

		startclock = SDL_GetTicks();
		deltaclock = startclock - oldclock;			// frame time = new time - previous
		oldclock = startclock;
		time_delta += deltaclock;					// time elapsed in render state
		// ------------------------------- GAME LOGIC -------------------------------------
		if (p1->lives <= 0){

			if (mode == 0){								// Player 1 dead in 1 player game				
				if (endtimer_flag == 0){
					endtimer_flag = 1;
					endtimer = SDL_GetTicks();
				}
				if (SDL_GetTicks() - endtimer > 3000){		
				
					high_scores(p1->score, 0, tex);
					endtimer_flag = 0;
					goto set_game_mode;
				}
			}
			else if (mode == 1 && p2->lives == 0){		// Player 1+2 dead in 2 player game
				if (endtimer_flag == 0){
					endtimer_flag = 1;
					endtimer = SDL_GetTicks();
				}
				if (SDL_GetTicks() - endtimer > 3000){			
					high_scores(p1->score, p2->score, tex);
					endtimer_flag = 0;
					goto set_game_mode;
				}
			}
			else if (mode == 2){						// Player 1 dead in 2 player duel
				if (endtimer_flag == 0){
					endtimer_flag = 1;
					endtimer = SDL_GetTicks();
				}
				if (SDL_GetTicks() - endtimer > 3000){	
					player_wins(2);
					goto set_game_mode;
				}
			}
		}

		if (mode == 2 && p2->lives <= 0){				// Player 2 dead in 2 player duel
			if (endtimer_flag == 0){
					endtimer_flag = 1;
					endtimer = SDL_GetTicks();
			}
			if (SDL_GetTicks() - endtimer > 3000){		
			
				player_wins(1);
				endtimer_flag = 0;
				goto set_game_mode;
			}
		}

		// end of level
		if (no_of_asteroids < 1 && mode != 2 && no_of_enemy < 1){
			if (endtimer_flag == 0){
				endtimer_flag = 1;
				endtimer = SDL_GetTicks();
			}
			if (SDL_GetTicks() - endtimer > 3000){
				++level;
				endtimer_flag = 0;
				init_particles();
				init_level(&field[0], &p[0], &en[0], level, mode);
				oldclock = SDL_GetTicks();
			}
		}
		// -----------------------------------PHYSICS-----------------------------
		while (time_delta >= DELTA_P){

			status = keyboard_events(p1, p2, mode);
			
			move_particles();

			if (status == 2){
				pause_game();
				oldclock = SDL_GetTicks();
			}
			if (status == 99)
				quit = 1;

			time_delta -= DELTA_P;
			
			// moving camera
			if (cam !=2 && mode == 0){
				camera.x -= p1->sh.velocity.x / 2.0f;
				camera.y -= p1->sh.velocity.y / 2.0f;
			}
			
			// 1 PLAYER MODE LOGIC
			if (p1->lives > 0){
				collision_detect(&field[0], p1, en);
				move_bullets(&p1->bulls[0]);
				
				if (p1->sh.shield == 1){
					Uint32 now = SDL_GetTicks();
					if (p1->sh.auto_shield_until){
						if (now >= p1->sh.auto_shield_until){
							p1->sh.shield = 0;
							p1->sh.auto_shield_until = 0;
						}
					}
					else if (now - p1->sh.shield_timer >= 100){
						p1->sh.shield_power -= 1;
						p1->sh.shield_timer = now;
						if (p1->sh.shield_power < 1.0){
							p1->sh.shield_power = 0;
							p1->sh.shield = 0;
						}
					}
				}
				// is player fixed at screen center?	
				if (cam > 0)
					move_ship(&p1->sh);
				else
					move_ship2(&p1->sh, &field[0], &p1->bulls[0], en);	
			}
	
			// 2 PLAYER MODE LOGIC
			if (mode > 0 && p2->lives > 0){
				
				move_ship(&p2->sh);
				move_bullets(&p2->bulls[0]);
				
				if (mode == 1){
					collision_detect(&field[0], p2, en);
				}
				
				if (mode == 2){
					dual_collisions(p1, p2);
				}
				
				if (p2->sh.shield == 1){
					Uint32 now = SDL_GetTicks();
					if (p2->sh.auto_shield_until){
						if (now >= p2->sh.auto_shield_until){
							p2->sh.shield = 0;
							p2->sh.auto_shield_until = 0;
						}
					}
					else if (now - p2->sh.shield_timer >= 100){
						p2->sh.shield_power -= 1;
						p2->sh.shield_timer = now;
						if (p2->sh.shield_power < 1)
							p2->sh.shield = 0;
					}
				}
			}	
			// not dual mode
			if (mode != 2){
				move_asteroid(&field[0]);			
			}
			// ENEMY PHYSICS
			if (no_of_enemy > 0){
						
				for (int i = 0; i < no_of_enemy; ++i){
					move_bullets(en[i].bulls);
				}	
				move_enemies(en, field, p, mode);
			}
		}	
		// ------------------------------------RENDER----------------------------------------------		
		if (no_of_powerups > 0){
			for (int i = 0; i < no_of_powerups; ++i){
				draw_powerup(&powerups[i]);
			}
		}
		draw_asteroids(field);

		if(p1->lives > 0){
			draw_ship(&p1->sh, 0);	
			if (p1->sh.shield == 1)
				draw_shield(&p1->sh);
			draw_bullets(p1->bulls);
		}

		draw_particles();

		if (mode > 0 && p2->lives > 0){ 
			draw_ship(&p2->sh, 1);
			if (p2->sh.shield == 1)
				draw_shield(&p2->sh);
			draw_bullets(p2->bulls);
		}
	
		if (no_of_enemy > 0){
			for (int i = 0; i < no_of_enemy; ++i){
				draw_ship(&en[i].sh, 2);
				if (en[i].sh.shield == 1)
					draw_enemy_shield(&en[i].sh);
				draw_bullets(en[i].bulls);
			}	
		}

		for (int i = 0; i < NO_OF_EXPLOSIONS; ++i){
			if (explode[i].age > 0.1){
				explode[i].age -= 0.75;
				draw_explosion(&explode[i]);
			}
		}		
			draw_background(tex);
			draw_scorebar(p, mode);
			if (screenshot_requested){
				save_screenshot_home();
				screenshot_requested = 0;
			}
			
			SDL_GL_SwapWindow(mywindow);
			glClear(GL_COLOR_BUFFER_BIT);
		//SDL_Delay(10);
	}
	close_sdl();
	end:
	return 0;
}	

void init_level(asteroid_t *field, Player_t *p, enemy_t *en, int level, int mode){

	no_of_enemy = 0;
	no_of_powerups = 0;
	dual_bullets = 1;
	cam = 1;
	int x;
	int no_of_players;
	camera.x = 0;
	camera.y = 0;

	if (mode > 0)
		no_of_players = 2;
	else
		no_of_players = 1;

	if (mode == 2)
		no_of_asteroids = 0;
	else
		no_of_asteroids = 8 + level;

	for (x = 0; x < no_of_players; ++x){
		if(level == 1){
			p[x].score = 0;
			p[x].lives = 5;
		}	
		init_player(&p[x], x, no_of_players);
	}

	if (level % 3 == 0){
		no_of_asteroids = 0;
		for(int i = 0; i < level; ++i){
			create_enemies(en, 1);
		}
	}

	for (int i = 0; i < no_of_asteroids; ++i){
		init_asteroid(&field[i], 3 * (1 + (i % 4)));
		do{
			field[i].position.x = (screen.width / no_of_asteroids) * (i + 0.5);
			field[i].position.y = ((float)rand()/(float)RAND_MAX) * screen.height;
		}while (field[i].position.y < screen.height/2 + 100 && field[i].position.y > screen.height/2 - 100);
	}

	level_transition(level);
}

void init_player(Player_t *p, int id, int num){

	if (p->lives == 0)
		return;
		
	p->sh.shield_power = 99;
	p->sh.shield_timer = SDL_GetTicks();
	p->sh.shield = 1;
	p->sh.auto_shield_until = SDL_GetTicks() + 1000;
	p->sh.velocity.x = 0;
	p->sh.velocity.y = 0;
	p->sh.gun = 0;

	if (cam == 0){
		p->sh.fire = create_bullet;
	}
	else{
		p->sh.fire = create_bullet_rel;
	}
	
	for (int i = 0; i < MAX_BULLETS; ++i)
		p->bulls[i].position.x = -99999;

	if (num == 2){
		if (id == 0){
			p->sh.position.x = screen.width/2 - 100;
			p->sh.position.y = screen.height/2;
			p->sh.angle = -PI;
		}
		if (id == 1){
			p->sh.position.x = screen.width/2 + 100;
			p->sh.position.y = screen.height/2;
			p->sh.angle = 0;
		}
	}		
	else {
		p->sh.position.x = screen.width/2;
		p->sh.position.y = screen.height/2;
		p->sh.angle = -PI/2;
	}

	for (int i = 0; i < 12; ++i){
		p->sh.thrust[i].velocity.x = 0;
		p->sh.thrust[i].velocity.y = 0;
		p->sh.thrust[i].position.x = 0;
		p->sh.thrust[i].position.y = 0;
		p->sh.thrust[i].state = 0;
		p->sh.thrust[i].age = 4.5;
	}
}

void init_asteroid(asteroid_t *a, float size){

	float speed;
	float direction;
	float world_scale = window_world_scale();

	a->angle = ((float)rand()/(float)RAND_MAX) * 2.0 * PI;	
	a->angle_inc = (((float)rand()/(float)RAND_MAX)-0.5)/15.0;
	a->has_collided = 0;
	a->size = size * world_scale;

	speed = ((float)rand()/(float)RAND_MAX) * 2 * world_scale;
	direction = ((float)rand()/(float)RAND_MAX) * 2.0 * PI;
	a->velocity.x = speed * cosf(direction);
	a->velocity.y = speed * sinf(direction);
}

void init_particles(){

	for (int i = 0; i < MAX_PARTICLES; ++i){
		parts[i].position.x = 0;
		parts[i].position.y = 0;
		float speed;
		float direction;
		parts[i].state = 0;
		parts[i].age = ((float)rand()/(float)RAND_MAX);
		parts[i].tint = 0;
		speed = ((float)rand()/(float)RAND_MAX) * 8 * screen.obj_scale_factor;
		direction = ((float)rand()/(float)RAND_MAX) * 2.0 * PI;
		parts[i].velocity.x = speed * cosf(direction);
		parts[i].velocity.y = speed * sinf(direction);
	}

	for (int i = 0; i < NO_OF_EXPLOSIONS; ++i){
		explode[i].x = 0;
		explode[i].y = 0;
		explode[i].age = 0;
	}
}

static void set_parts_tinted(int x, int y, Uint8 tint){

	// simple particle engine that handles NO_OF_EXPLOSIONS simultaneous events
	
	static int id = 0;
	int chunk = MAX_PARTICLES / NO_OF_EXPLOSIONS;		// number of particles per explosion
	id += 1;
	id = id % NO_OF_EXPLOSIONS;							// Get the next available chunk of particles
	
	explode[id].x = x;									// and set the origin and lifespan
	explode[id].y = y;
	explode[id].age = 4.5;

	for (int i = chunk * id; i < chunk * (id + 1); ++i){
		parts[i].position.x = x;
		parts[i].position.y = y;
		parts[i].state = 1;
		parts[i].age = ((float)rand()/(float)RAND_MAX);
		parts[i].tint = tint;
	}		
}

void set_parts(int x, int y){

	set_parts_tinted(x, y, 0);
}

void set_enemy_parts(int x, int y){

	set_parts_tinted(x, y, 1);
}

void init_sound(){

	int audio_rate = 44100;
	Uint16 audio_format = AUDIO_S16SYS;
	int audio_channels = 2;
	int audio_buffers = 1024;
   
    SDL_Init(SDL_INIT_AUDIO);

	if (Mix_OpenAudio(audio_rate, audio_format, audio_channels, audio_buffers)) {
    	SDL_Log("Unable to open audio!\n");
    	exit(1);
  	}
  	Mix_AllocateChannels(32);
  	Mix_ReserveChannels(1); 			// reserve channel zero for shield
  	
	sound[CRASH].effect = Mix_LoadWAV( "./data/crash.wav" );
	sound[THRUST].effect = Mix_LoadWAV( "./data/thrust.wav" );
	sound[FIRE].effect = Mix_LoadWAV( "./data/fire.wav" );
	sound[HIT].effect = Mix_LoadWAV( "./data/hit.wav" );
	sound[SHIELD].effect = Mix_LoadWAV( "./data/shield.wav" );
	sound[IMPACT].effect = Mix_LoadWAV( "./data/impact.wav" );
	sound[SELECTS].effect = Mix_LoadWAV( "./data/select.wav" );
	sound[BOUNCE].effect = Mix_LoadWAV( "./data/bounce.wav" );
	sound[HYPER].effect = Mix_LoadWAV( "./data/hyper.wav" );
	sound[ENTRY].effect = Mix_LoadWAV( "./data/entry.wav" );
	sound[POWER].effect = Mix_LoadWAV( "./data/power.wav" );

	Mix_QuerySpec(&music_sample_rate, &audio_format, &music_channels);
	if (audio_format != AUDIO_S16SYS || music_channels != 2){
		SDL_Log("Audio format incompatible with module playback. Expected S16 stereo.");
		music_enabled = 0;
		return;
	}

	music_enabled = 1;
	SDL_AtomicSet(&music_track_finished, 0);
	Mix_HookMusic(music_hook, NULL);
	set_sfx_volume(SFX_VOLUME);
	set_music_volume(MUSIC_VOLUME);
	if (!load_random_module()){
		SDL_Log("No module files found in ./mods");
	}
}

void set_sfx_volume(int volume){

	if (volume < 0)
		volume = 0;
	if (volume > MIX_MAX_VOLUME)
		volume = MIX_MAX_VOLUME;
	SFX_VOLUME = volume;
	Mix_Volume(-1, SFX_VOLUME);
}

void set_music_volume(int volume){

	if (volume < 0)
		volume = 0;
	if (volume > MIX_MAX_VOLUME)
		volume = MIX_MAX_VOLUME;
	MUSIC_VOLUME = volume;
}

void update_music_player(void){

	if (!music_enabled)
		return;
	if (!SDL_AtomicGet(&music_track_finished))
		return;
	SDL_AtomicSet(&music_track_finished, 0);
	load_random_module();
}

void shutdown_music_player(void){

	Mix_HookMusic(NULL, NULL);
	music_enabled = 0;
	SDL_AtomicSet(&music_track_finished, 0);
	clear_current_music();
}

void init_thruster(ship_t *sh){

	float posx = sh->position.x - 12*cosf(sh->angle);
	float posy = sh->position.y - 12*sinf(sh->angle);
	float speed;
	float direction;

	for (int i = 0; i < 12; ++i){
		sh->thrust[i].position.x = posx;
		sh->thrust[i].position.y = posy;
		sh->thrust[i].state = 1;
		sh->thrust[i].age = ((float)rand()/(float)RAND_MAX);
		speed = screen.obj_scale_factor * ((float)rand()/(float)RAND_MAX);
		direction = ((float)rand()/(float)RAND_MAX)/2 - 0.25;
		direction = sh->angle + PI + direction;
		sh->thrust[i].velocity.x = speed * cosf(direction);
		sh->thrust[i].velocity.y = speed * sinf(direction);
	}
}
// bullets that move relative to ships velocity
void create_bullet_rel(bullet_t *bulls, ship_t *sh){
	
	float speed = 10 * screen.obj_scale_factor;
	float shx, shy, vx, vy, m, dot;
	float dual_offset = -0.2;
	
	if (sh->gun == 1){

		for (int i = 0; i < MAX_BULLETS; ++i){		// scan through bullets until we find one that is free

			if (bulls[i].position.x == -99999){

				shx = speed * cosf(sh->angle + dual_offset);
				shy = speed * sinf(sh->angle + dual_offset);
				bulls[i].position.x = sh->position.x + 3 * shx;
				bulls[i].position.y = sh->position.y + 3 * shy;

				shx = screen.obj_scale_factor * speed * cosf(sh->angle);
				shy = screen.obj_scale_factor * speed * sinf(sh->angle);
				m = sqrtf(shx*shx + shy*shy);
				vx = shx/m;
				vy = shy/m;
				dot = vx * sh->velocity.x + vy * sh->velocity.y;
				vx = vx * dot;
				vy = vy * dot;
				bulls[i].velocity.x = shx + sh->velocity.x;
				bulls[i].velocity.y = shy + sh->velocity.y;

				if (dual_offset > 0){
					play_sound(FIRE, 0);
					return;
				}
				dual_offset = 0.2;
			}
		}
	}

	for (int i = 0; i < MAX_BULLETS; ++i){		// scan through bullets until we find one that is free
		
		if (bulls[i].position.x == -99999){
			play_sound(FIRE, 0);
			shx = speed * cosf(sh->angle);
			shy = speed * sinf(sh->angle);
			bulls[i].position.x = sh->position.x + 3 * shx;
			bulls[i].position.y = sh->position.y + 3 * shy;
			m = sqrtf(shx*shx + shy*shy);
			vx = shx/m;
			vy = shy/m;
			dot = vx * sh->velocity.x + vy * sh->velocity.y;
			vx = vx * dot;
			vy = vy * dot; 
			bulls[i].velocity.x = shx + sh->velocity.x;
			bulls[i].velocity.y = shy + sh->velocity.y;
			
			return;
		}
	}
	return;
}
// bullets that move relative to screen		
void create_bullet(bullet_t *bulls, ship_t *sh){

	
	float speed = screen.obj_scale_factor * 10;
	float shx, shy;
	
	for (int i = 0; i < MAX_BULLETS; ++i){
		
		if (bulls[i].position.x == -99999){
			play_sound(FIRE, 0);
			shx = speed * cosf(sh->angle);
			shy = speed * sinf(sh->angle);
			bulls[i].position.x = sh->position.x + 3 * shx;
			bulls[i].position.y = sh->position.y + 3 * shy;
			bulls[i].velocity.x = shx;
			bulls[i].velocity.y = shy;
			
			return;
		}
	}
	return;
}

void init_powerup(float x, float y){

	if (no_of_powerups == MAX_POWERUPS)
		return;

	powerups[no_of_powerups].position.x = x;
	powerups[no_of_powerups].position.y = y;
	powerups[no_of_powerups].age = 0;

	float rn = ((float)rand()/(float)RAND_MAX);
	if (rn > 0.75)
		powerups[no_of_powerups].type = 0;
	else if (rn > 0.5)
		powerups[no_of_powerups].type = 1;
	else if (rn > 0.25)
		powerups[no_of_powerups].type = 2;
	else
		powerups[no_of_powerups].type = 3;

	no_of_powerups += 1;
}

void move_asteroid(asteroid_t *field){

	// For each asteroid in the field update its coordinates.
	for (int i = 0; i < no_of_asteroids; ++i){

		field[i].angle = field[i].angle + field[i].angle_inc;

		// check for boundry conditions
		
		if (field[i].position.x > screen.width + field[i].size * 5){
			field[i].position.x = -field[i].size * 5;
		}
		if (field[i].position.x < 0 - field[i].size * 5){
			field[i].position.x = screen.width + field[i].size * 5;
		}
		if (field[i].position.y > screen.height + field[i].size * 5){
			field[i].position.y = - field[i].size * 5;
		}
		if (field[i].position.y < 0 - field[i].size * 5){
			field[i].position.y = screen.height + field[i].size * 5;
		}
		// update position
		field[i].position.x = field[i].position.x + field[i].velocity.x;
		field[i].position.y = field[i].position.y + field[i].velocity.y;
	}	
}

void move_ship(ship_t *sh){
	
	if (sh->position.x > screen.width ){
		sh->position.x = 0 ;
	}
	if (sh->position.x < 0 ){
		sh->position.x = screen.width ;
	}
	if (sh->position.y > screen.height ){
		sh->position.y = 0 ;
	}
	if (sh->position.y < 0 ){
		sh->position.y = screen.height ;
	}
	sh->position.x = sh->position.x + sh->velocity.x;
	sh->position.y = sh->position.y + sh->velocity.y;

	for (int i= 0; i < 12; ++i){
		if (sh->thrust[i].state == 1){	
			sh->thrust[i].position.x += sh->thrust[i].velocity.x;
			sh->thrust[i].position.y += sh->thrust[i].velocity.y;
			sh->thrust[i].age -= 0.01;
			if (sh->thrust[i].age < 0.1){
				sh->thrust[i].state = 0;
			}
		}
	}
}
// ship remains fixed and all other objects are moved relative
void move_ship2(ship_t *sh, asteroid_t *field, bullet_t *bulls, enemy_t *en){

	for (int i = 0; i < no_of_asteroids; ++i){
		field[i].position.x += -sh->velocity.x;
		field[i].position.y += -sh->velocity.y;
	}
	for (int i = 0; i < MAX_BULLETS; ++i){
		if (bulls[i].position.x != -99999){
			//bulls[i].position.x += -sh->velocity.x * DELTA;
			//bulls[i].position.y += -sh->velocity.y * DELTA;
			//bulls[i].position.x += bulls[i].velocity.x;
			//bulls[i].position.y += bulls[i].velocity.y;
		}
	}
	for (int i = 0; i < no_of_enemy; ++i){
		en[i].sh.position.x += -sh->velocity.x;
		en[i].sh.position.y += -sh->velocity.y;
		for (int j = 0; j < MAX_BULLETS; ++j){
			if (bulls[j].position.x != -99999){
				bulls[j].position.x += -sh->velocity.x;
				bulls[j].position.y += -sh->velocity.y;
			}
		}
	}
}

void move_bullets(bullet_t *bulls){

	for (int i = 0; i < MAX_BULLETS; ++i){
		if (bulls[i].position.x < 0 || bulls[i].position.x > screen.width)
			bulls[i].position.x = -99999;

		else if (bulls[i].position.y < 0 || bulls[i].position.y > screen.height)
			bulls[i].position.x = -99999; // sentinel value

		else{
			bulls[i].position.x += bulls[i].velocity.x;
			bulls[i].position.y += bulls[i].velocity.y;
		}

	}
}

void move_particles(){

	for (int i = 0; i < MAX_PARTICLES; ++i){
		if (parts[i].state == 1){
			parts[i].position.x += parts[i].velocity.x;
			parts[i].position.y += parts[i].velocity.y;
			parts[i].age -= 0.001;
			if (parts[i].age < 0.01){
				parts[i].state = 0;
			}
		}	
	}
}

void dual_collisions(Player_t *p1, Player_t *p2){

	float xdiff, ydiff;
	float distance2;
	
	// distance between the ships

	xdiff =  p1->sh.position.x - p2->sh.position.x;
	ydiff =  p1->sh.position.y - p2->sh.position.y;
	distance2 = xdiff * xdiff + ydiff * ydiff;
	
	if (distance2 < 550.0 * screen.obj_scale_factor){
		if (p1->sh.shield == 1 && p2->sh.shield == 1){	// bounce players
			collide2(&p1->sh.velocity, &p1->sh.position, 1, &p2->sh.velocity, &p2->sh.position, 1);
			play_sound(BOUNCE, 0);
		}
		if (p2->sh.shield == 0){	// p2 dies
			play_sound(IMPACT, 0);				
			--p2->lives;
			set_parts(p2->sh.position.x, p2->sh.position.y);
			init_player(p2, 1, 2);
		}
		if (p1->sh.shield == 0){	// p1 dies
			play_sound(IMPACT, 0);				
			--p1->lives;
			set_parts(p1->sh.position.x, p1->sh.position.y);
			init_player(p1, 0, 2);
		}
	}

	// player bullet collisions
	// compare p1 bullets to p2
	for (int j = 0; j < MAX_BULLETS; ++j){
		if (p1->bulls[j].position.x != -99999){
			xdiff = p1->bulls[j].position.x - p2->sh.position.x;
			ydiff = p1->bulls[j].position.y - p2->sh.position.y;
			distance2 = xdiff * xdiff + ydiff * ydiff;

			if (distance2 < 120.0f * screen.obj_scale_factor){
				p1->bulls[j].position.x = -99999;
				if (p2->sh.shield != 1){
					play_sound(HIT, 0);
					set_parts(p2->sh.position.x, p2->sh.position.y);	
					--p2->lives;
					init_player(p2, 1, 2);
				}
				else collide2(&p1->bulls[j].velocity, &p1->bulls[j].position, 1, &p2->sh.velocity, &p2->sh.position, 40);
			}
		}
	}	
	// compare p2 bullets to p1
	for (int j = 0; j < MAX_BULLETS; ++j){
		if (p2->bulls[j].position.x != -99999){
			xdiff = p2->bulls[j].position.x - p1->sh.position.x;
			ydiff = p2->bulls[j].position.y - p1->sh.position.y;
			distance2 = xdiff * xdiff + ydiff * ydiff;

			if (distance2 < 120.0f * screen.obj_scale_factor){
				p2->bulls[j].position.x = -99999;
				if (p1->sh.shield != 1){
					play_sound(HIT, 0);
					set_parts(p1->sh.position.x, p1->sh.position.y);	
					--p1->lives;
					init_player(p1, 0, 2);
				}
				else collide2(&p2->bulls[j].velocity, &p2->bulls[j].position, 1, &p1->sh.velocity, &p1->sh.position, 40);
			}
		}
	}	
}

void collision_detect(asteroid_t *field, Player_t *p, enemy_t *en){
	
	float xdiff, ydiff;
	float distance2 = 0, overlap = 0, xpos = 0, ypos = 0, size = 0;
	const float asteroid_radius = 4.5f;
	
	for (int i = 0; i < no_of_asteroids - 1; ++i){		// each asteroid
		xpos = field[i].position.x;
		ypos = field[i].position.y;
		size = field[i].size * asteroid_radius;
		for (int j = i + 1; j < no_of_asteroids; ++j){	// compare to each successive asteroid
			xdiff =  xpos - field[j].position.x;
			ydiff =  ypos - field[j].position.y;
			distance2 = sqrt(xdiff * xdiff + ydiff * ydiff);
			//overlap = distance2 - size * size - field[j].size * field[j].size * 25 - 10 * size * field[j].size;
			//if (overlap <= 0){
			if (distance2 < (size + field[j].size * asteroid_radius)){
				//if (check_for_collision(i, j)){
					collide2(&field[i].velocity, &field[i].position, field[i].size, &field[j].velocity, &field[j].position, field[j].size);	
					play_sound(CRASH, 0);
				//}
			}
		}
	}
			
	for (int i = 0; i < no_of_asteroids; ++i){			// player - asteroid
		// compare ship
		xdiff =  p->sh.position.x - field[i].position.x;
		ydiff =  p->sh.position.y - field[i].position.y;
		distance2 = xdiff * xdiff + ydiff * ydiff;
		overlap = distance2 - field[i].size * field[i].size * asteroid_radius * asteroid_radius - 144 - 2 * field[i].size * asteroid_radius * 10;
		if (overlap <= 910.0f * screen.obj_scale_factor){
			if (p->sh.shield == 1){
				collide2(&field[i].velocity, &field[i].position, field[i].size, &p->sh.velocity, &p->sh.position, 1);
				play_sound(BOUNCE, 0);
			}
			else if (overlap <= 0){
				play_sound(IMPACT, 0);
				set_parts(p->sh.position.x, p->sh.position.y);			
				--p->lives;
				init_player(p, 1, 1);
			}
		}
		
		for (int j = 0; j < MAX_BULLETS; ++j){			// bullet asteroid
			int hit = 0; 						// flag if asteroid already hit
			// compare bullet
			if (p->bulls[j].position.x != -99999){
				xdiff = p->bulls[j].position.x - field[i].position.x;
				ydiff = p->bulls[j].position.y - field[i].position.y;
				distance2 = xdiff * xdiff + ydiff * ydiff;
				overlap = distance2 - field[i].size * field[i].size * asteroid_radius * asteroid_radius;
				if (overlap <= 0){
					split_asteroid(&field[0], i);
					if (((float)rand()/(float)RAND_MAX) > 0.8){				
						create_enemies(en, 1);
					}
					p->score = p->score + field[i].size;
					play_sound(HIT, 0);
					p->bulls[j].position.x = -99999;
					hit = 1;
				}
				
			}
			if (hit == 1)
				break;
		}
	}

	for (int i = 0; i < no_of_powerups; ++i){			// player - powerup

		xdiff =  p->sh.position.x - powerups[i].position.x;
		ydiff =  p->sh.position.y - powerups[i].position.y;
		distance2 = xdiff * xdiff + ydiff * ydiff;

		if (distance2 <= 256.0f * screen.obj_scale_factor){
			collect_powerup(en, p, i);
		}
	}

	if (no_of_enemy == 0)
		return;
	
	for (int i = 0; i < no_of_enemy; ++i){
		for (int j = 0; j < MAX_BULLETS; ++j){			// enemy bullet player
		// compare bullet
			if (en[i].bulls[j].position.x != -99999){	// if bullet is existing in game
				xdiff = en[i].bulls[j].position.x - p->sh.position.x;
				ydiff = en[i].bulls[j].position.y - p->sh.position.y;
				distance2 = xdiff * xdiff + ydiff * ydiff;
				//overlap = distance2 - 100;
				if (distance2 < 120.0f * screen.obj_scale_factor){
					en[i].bulls[j].position.x = -99999;
					if (p->sh.shield != 1){
						play_sound(HIT, 0);
						set_parts(p->sh.position.x, p->sh.position.y);	
						--p->lives;
						init_player(p, 1, 1);
					}
				}
			}
		}
	}

	for (int j = 0; j < MAX_BULLETS; ++j){			// player bullet enemy
		if (p->bulls[j].position.x != -99999){	
			for ( int k = 0; k < no_of_enemy; ++k){
				xdiff = p->bulls[j].position.x - en[k].sh.position.x;
				ydiff = p->bulls[j].position.y - en[k].sh.position.y;
				distance2 = xdiff * xdiff + ydiff * ydiff;
				// Enemy may proactively shield against incoming fire (25% chance).
				if (en[k].sh.shield == 0 && en[k].shield_uses > 0){
					float dx = en[k].sh.position.x - p->bulls[j].position.x;
					float dy = en[k].sh.position.y - p->bulls[j].position.y;
					float closing = dx * p->bulls[j].velocity.x + dy * p->bulls[j].velocity.y;
					float shield_chance = (no_of_asteroids == 0) ? 0.50f : 0.25f;
					// Last-resort trigger: bullet is close and approaching.
					if (closing > 20.0f && distance2 < 180.0f * screen.obj_scale_factor){
						if (((float)rand()/(float)RAND_MAX) < shield_chance){
							en[k].sh.shield = 1;
							en[k].shield_uses -= 1;
							en[k].shield_until = SDL_GetTicks() + 4000;
						}
					}
				}
					if (distance2 < 140.0f * screen.obj_scale_factor){
						if (en[k].sh.shield == 1){
							p->bulls[j].position.x = -99999;
						}
						else{
							kill_enemy(en, k);
						p->score += 25;
						p->bulls[j].position.x = -99999;
					}
				}
			}
		}	
	}
	
	for (int i = 0; i < no_of_enemy; ++i){			// player - enemy
		
		xdiff =  p->sh.position.x - en[i].sh.position.x;
		ydiff =  p->sh.position.y - en[i].sh.position.y;
		distance2 = xdiff * xdiff + ydiff * ydiff;
		
		if (distance2 <= 200.0f * screen.obj_scale_factor){
			if (p->sh.shield == 1 && en[i].sh.shield == 1){
				collide2(&p->sh.velocity, &p->sh.position, 1, &en[i].sh.velocity, &en[i].sh.position, 1);
				play_sound(BOUNCE, 0);
			}
			else if (p->sh.shield == 1 && en[i].sh.shield == 0){
				kill_enemy(en, i);
				p->score += 25;
			}
			else if (p->sh.shield == 0 && en[i].sh.shield == 1){
				play_sound(IMPACT, 0);
				set_parts(p->sh.position.x, p->sh.position.y);
				--p->lives;
				init_player(p, 1, 1);
			}
			else {
				play_sound(IMPACT, 0);
				kill_enemy(en, i);			
				--p->lives;
				init_player(p, 1, 1);
			
			}
		}
	}
} 	// end collision_detect

// Take 2 position Vectors and two velocity vectors, do elastic collision
// todo consider when we seperate two objects that we move them into another object
void collide2(Vector *v1, Vector *p1, int s1, Vector *v2, Vector *p2, int s2){

	Vector colnormal;
	Vector colplane;
	Vector diff;
	
	float mag, overlap = 0;

	while (overlap < 1){
		diff.x = p2->x - p1->x;
		diff.y = p2->y - p1->y;
		mag = diff.x*diff.x + diff.y*diff.y;
		overlap = mag - s1 * s1 * 25 - s2 * s2 * 25 - 10 * s1 * s2;

		if (p1->x < p2->x){
			--p1->x;
			++p2->x;
		}
		else{
			++p1->x;
			--p2->x;
		}
		if (p1->y < p2->y){
			--p1->y;
			++p2->y;
		}
		else{
			++p1->y;
			--p2->y;
		}
	}	
	
	diff.x = p2->x - p1->x;
	diff.y = p2->y - p1->y;
	
	mag = sqrtf(diff.x*diff.x + diff.y*diff.y);
	// get normalised collision plane between objects
	colnormal.x = diff.x/mag;
	colnormal.y = diff.y/mag;
	// normal vector = swapping the two components of the vector and multiplying the first by -1.
	colplane.x = -1.0 * colnormal.y;
	colplane.y = colnormal.x;
	
	float n_veli = v1->x * colnormal.x + v1->y * colnormal.y;
	float c_veli = v1->x * colplane.x + v1->y * colplane.y;

	float n_velj = v2->x * colnormal.x + v2->y * colnormal.y;
	float c_velj = v2->x * colplane.x + v2->y * colplane.y;

	// Calculate the screen.obj_scale_factorr velocities of each object after the collision.

	float new1 = (n_veli * (s1 - s2) + 2 * s2 * n_velj) / (s1 + s2);
	float new2 = (n_velj * (s2 - s1) + 2 * s1 * n_veli) / (s1 + s2);
	
	// Convert the screen.obj_scale_factorrs to vectors by multiplying by the normalised plane vectors.
	v1->x = colnormal.x * new1 + colplane.x * c_veli;
	v1->y = colnormal.y * new1 + colplane.y * c_veli;
	
	v2->x = colnormal.x * new2 + colplane.x * c_velj;
	v2->y = colnormal.y * new2 + colplane.y * c_velj;
}


Vector v_sub(Vector a, Vector b){

	Vector result;
	result.x = a.x - b.x;
	result.y = a.y - b.y;
	return result;
}
float v_dot(Vector a, Vector b){

	return a.x * b.x + a.y * b.y;
}

// accurate triangle point collision
// Test if point p intersects triangle abc
// https://blackpawn.com/texts/pointinpoly/default.html
static inline int test_triangle_point(Vector a, Vector b, Vector c, Vector p){

	float u, v;
	//// Compute vectors        
	Vector v0 = v_sub(c, a);
	Vector v1 = v_sub(b, a);
	Vector v2 = v_sub(p, a);

	// Compute dot products
	float dot00 = v_dot(v0, v0);
	float dot01 = v_dot(v0, v1);
	float dot02 = v_dot(v0, v2);
	float dot11 = v_dot(v1, v1);
	float dot12 = v_dot(v1, v2);

	// Compute barycentric coordinates
	float invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
	u = (dot11 * dot02 - dot01 * dot12) * invDenom;
	v = (dot00 * dot12 - dot01 * dot02) * invDenom;

	// Check if point is in triangle
	return (u >= 0) && (v >= 0) && ((u + v) < 1);
}

static int check_for_collision(int x, int y){

	// for each triangle in asteroid x check each point in asteroid y for intersection
	// todo need to have rotated all asteroid verts
	Vector a, b, c, p;
	float xpos = field[x].position.x;
	float ypos = field[x].position.y;
	float x2pos = field[y].position.x;
	float y2pos = field[y].position.y;
	a.x = asteroid_offsets[0] + xpos;
	a.y = asteroid_offsets[1] + ypos;
	
	int triangles = ASTEROID_VERTS / 2;
	int points = ASTEROID_VERTS / 2 - 2;
	for (int j = 0; j < points; j += 2){
		p.x = asteroid_offsets[j + 2] + x2pos;
		p.y = asteroid_offsets[j + 3] + y2pos;
		for (int i = 2; i < triangles; i += 2){
			b.x = asteroid_offsets[i] + xpos;
			b.y = asteroid_offsets[1 + i] + ypos;
			c.y = asteroid_offsets[2 + i] + xpos;
			c.y = asteroid_offsets[3 + i] + ypos;
			if (test_triangle_point(a, b, c, p)){
			printf("detected\n");
				return 1;
			}
		}
	}
	return 0;
}

void split_asteroid(asteroid_t *field, int a){

	set_parts(field[a].position.x, field[a].position.y);
	if (((float)rand()/(float)RAND_MAX) < POWERUP_PROBABILITY)
		init_powerup(field[a].position.x, field[a].position.y);

	float a_speed, a_direction, dx, dy;
	float b_speed, b_direction;
	
	if (field[a].size <= 3 * window_world_scale()){
		memmove(&field[a], &field[no_of_asteroids-1], sizeof(asteroid_t));
		no_of_asteroids -= 1;
		return;
	}

	int n = no_of_asteroids++;
	float oldx = field[a].position.x;
	float oldy = field[a].position.y;

	a_direction = atan2(field[a].velocity.y, field[a].velocity.x) + PI/4;
	a_speed = sqrtf(field[a].velocity.x * field[a].velocity.x + field[a].velocity.y * field[a].velocity.y);
	b_direction = atan2(field[a].velocity.y, field[a].velocity.x) - PI/4;
	b_speed = sqrtf(field[a].velocity.x * field[a].velocity.x + field[a].velocity.y * field[a].velocity.y);

	// first new asteroid
	field[a].size /= 2.0f;	
	field[a].velocity.x = a_speed * cosf(a_direction);
	field[a].velocity.y = a_speed * sinf(a_direction);
		
	dx = field[a].velocity.x / a_speed;
	dy = field[a].velocity.y / a_speed;
	
	field[a].position.x = oldx + dx * field[a].size * 7 + 2 * dx;
	field[a].position.y = oldy + dy * field[a].size * 7 + 2 * dy;

	// second new asteroid
	field[n].size = field[a].size;
	field[n].velocity.x = b_speed * cosf(b_direction);
	field[n].velocity.y = b_speed * sinf(b_direction);
		
	dx = field[n].velocity.x / b_speed;
	dy = field[n].velocity.y / b_speed;
	
	field[n].position.x = oldx + dx * field[n].size * 7 + 2 * dx;
	field[n].position.y = oldy + dy * field[n].size * 7 + 2 * dy;	
}

Vector min_seperate(Vector p1, int s1, Vector p2, int s2){

	Vector res;
	float angle, m;
	 
	res.x = p2.x - p1.x;
	res.y = p2.y - p1.y;
	angle = atan2f(res.y, res.x);
	
	m = sqrtf(res.x*res.x + res.y*res.y);
	m = fabs(m - s1 - s2);

	res.x = m * cosf(angle);
	res.y = m * sinf(angle);

	return (res);
}
	
void play_sound(int index, int loop){

	if (index < 0 || index >= MAX_SOUNDS || sound[index].effect == NULL)
		return;

	if(index == SHIELD)
		Mix_PlayChannel(0, sound[index].effect, loop);
	else if (index != SHIELD)
		Mix_PlayChannel(-1, sound[index].effect, loop);
}

int pause_game(){

	texture_t t;
	SDL_Event e;
	int q = 0;

	if (render == 1){

		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		Mix_PauseMusic();
		Mix_HaltChannel(-1);
		render_text(&t, "PAUSED", 1, 88);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, t.t);
		glColor4f( 1.0, 0.55, 0.1, 1.0 );

		GLfloat tex[] = {0,0, 1,0, 1,1, 0,1};
		GLfloat box[] = {screen.width/2 - t.w/2, screen.height/2 - t.h,
						 screen.width/2 + t.w/2, screen.height/2 - t.h,
						 screen.width/2 + t.w/2, screen.height/2,
						 screen.width/2 - t.w/2, screen.height/2};

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		glVertexPointer(2, GL_FLOAT, 0,box);
		glTexCoordPointer(2, GL_FLOAT, 0, tex);
		glDrawArrays(GL_TRIANGLE_FAN,0,4);
		SDL_GL_SwapWindow(mywindow);

	}
		
	while (!q){
		SDL_PollEvent(&e);
	
		if (e.type == SDL_KEYDOWN)
			q = 1;
		if (e.type == SDL_QUIT){
			glDeleteTextures(1, &t.t);
			close_sdl();
		}
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);

	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &t.t);
	Mix_ResumeMusic();

	return 0;
}

// returns 0 = normal; 1 = quit; 2 = game was paused.
int keyboard_events(Player_t *p1, Player_t *p2, int mode){

	SDL_Event e;
	int mm_flag = 1;

	while (SDL_PollEvent(&e) != 0){	
		
			if (e.type == SDL_KEYDOWN && e.key.repeat == 0){

					if (e.key.keysym.sym == SDLK_F12){
						screenshot_requested = 1;
						continue;
					}

				if (e.key.keysym.sym == SDLK_p){
					pause_game();
					return 2;
			}
			
			// PLAYER 1 CONTROLS - KEYDOWN		
			if (p1->lives > 0){

				if(e.key.keysym.sym == layout[P1FIRE]){
					if (!p1->sh.shield){
						p1->sh.fire(&p1->bulls[0], &p1->sh);
					}
				}
		
				else if(e.key.keysym.sym == layout[P1THRUST]){
					if (!p1->sh.shield){
						play_sound(THRUST, 0);
						init_thruster(&p1->sh);
					}
				}
		
					else if(e.key.keysym.sym == layout[P1SHIELD]){
						if (p1->sh.shield_power > 0){
							p1->sh.shield = 1;
							p1->sh.shield_timer = SDL_GetTicks();
							p1->sh.auto_shield_until = 0;
							play_sound(SHIELD, -1);
						}
					}
			}
				
			// PLAYER 2 CONTROLS - KEYDOWN
			if (mode > 0 && p2->lives > 0){
	
				if(e.key.keysym.sym == layout[P2FIRE]){
					if (!p2->sh.shield){
						p2->sh.fire(&p2->bulls[0], &p2->sh);
					}
				}
				else if(e.key.keysym.sym == layout[P2THRUST]){
					if (!p2->sh.shield){
						play_sound(THRUST, 0);
						init_thruster(&p2->sh);
					}
				}
					else if(e.key.keysym.sym == layout[P2SHIELD]){
						if (p2->sh.shield_power > 0){
							p2->sh.shield = 1;
							p2->sh.shield_timer = SDL_GetTicks();
							p2->sh.auto_shield_until = 0;
							play_sound(SHIELD, -1);
						}
					}
			}
		}	// end key down events
	
		if (e.type == SDL_KEYUP){
			// PLAYER 1 KEYUPS
				if(e.key.keysym.sym == layout[P1SHIELD]){	
					p1->sh.shield = 0;
					p1->sh.shield_timer = 0;
					p1->sh.auto_shield_until = 0;
					Mix_HaltChannel(0);
				}
			// PLAYER 2 KEYUPS
			if (mode > 0){
					if (e.key.keysym.sym == layout[P2SHIELD]){
						p2->sh.shield = 0;
						p2->sh.shield_timer = 0;
						p2->sh.auto_shield_until = 0;
						Mix_HaltChannel(0);
					}
				}
		}

		if (layout[MOUSE_ENABLE] == 1 && p1->lives > 0){			
			if (e.type == SDL_MOUSEBUTTONDOWN){
				if (e.button.button == SDL_BUTTON_LEFT){
					if (!p1->sh.shield){
						p1->sh.fire(&p1->bulls[0], &p1->sh);
					}
				}				
				if (e.button.button == SDL_BUTTON_RIGHT){
					if (!p1->sh.shield){
						play_sound(THRUST, 0);
						init_thruster(&p1->sh);
						p1->sh.velocity.x += screen.ctl_scale_factor * cosf(p1->sh.angle) * 1.5;
						p1->sh.velocity.y += screen.ctl_scale_factor * sinf(p1->sh.angle) * 1.5;
					}
				}
					if (e.button.button == SDL_BUTTON_MIDDLE){
						if (p1->sh.shield_power > 0){
							p1->sh.shield = 1;
							p1->sh.shield_timer = SDL_GetTicks();
							p1->sh.auto_shield_until = 0;
							play_sound(SHIELD, -1);
						}
					}
			}
			if (e.type == SDL_MOUSEBUTTONUP){
					if (e.button.button == SDL_BUTTON_MIDDLE){
						p1->sh.shield = 0;
						p1->sh.shield_timer = 0;
						p1->sh.auto_shield_until = 0;
						Mix_HaltChannel(0);
					}
				}		
			if ((e.type == SDL_MOUSEMOTION) && mm_flag){
				if (e.motion.xrel < 0){
					p1->sh.angle += 0.75 * e.motion.xrel / screen.h_dpi;
				}
				else if (e.motion.xrel > 0){
					p1->sh.angle += 0.75 * e.motion.xrel / screen.h_dpi;
				}
				--mm_flag;
			}
		}
	} // end non repeating control events

	// Repeating events player 1
	if (layout[MOUSE_ENABLE] == 1){
		if (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT)){
			if (!p1->sh.shield){
				p1->sh.velocity.x += screen.ctl_scale_factor * cosf(p1->sh.angle) * 1.5;
				p1->sh.velocity.y += screen.ctl_scale_factor * sinf(p1->sh.angle) * 1.5;
			}
		}
	}
			
	const Uint8* currentKeyStates = SDL_GetKeyboardState(NULL);
	if (currentKeyStates[SDL_GetScancodeFromKey(layout[P1THRUST])]){
		if (!p1->sh.shield){
			p1->sh.velocity.x += screen.ctl_scale_factor * cosf(p1->sh.angle) * 1.5;
			p1->sh.velocity.y += screen.ctl_scale_factor * sinf(p1->sh.angle) * 1.5;
		}	
	}
	
	if (currentKeyStates[SDL_GetScancodeFromKey(layout[P1LEFT])]){
		p1->sh.angle -= 0.05; 
	}
	if (currentKeyStates[SDL_GetScancodeFromKey(layout[P1RIGHT])]){
		p1->sh.angle += 0.05;
	}
	if (currentKeyStates[SDL_SCANCODE_ESCAPE]){
		return 99;
	}
	
	// Player 2 controls repeating
	if (mode > 0){
		if (currentKeyStates[SDL_GetScancodeFromKey(layout[P2THRUST])]){
			if (!p2->sh.shield){
				p2->sh.velocity.x += screen.ctl_scale_factor * cosf(p2->sh.angle) * 1.5;
				p2->sh.velocity.y += screen.ctl_scale_factor * sinf(p2->sh.angle) * 1.5;
			}
		}
		if (currentKeyStates[SDL_GetScancodeFromKey(layout[P2LEFT])]){
			p2->sh.angle -= 0.05; 
		}
		if (currentKeyStates[SDL_GetScancodeFromKey(layout[P2RIGHT])]){
			p2->sh.angle += 0.05;
		}
	}	
	return 0;
}

void player_wins(int n){

	texture_t t;
	SDL_Event e;
	int q = 0;
	char winner[15];

	play_sound(HYPER, 0);
	sprintf(winner, "PLAYER %d WINS", n);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	render_text(&t, winner, 1, 88);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,1);
	glBindTexture(GL_TEXTURE_2D, t.t);
	glColor4f( 0.1, 0.55, 1, 1.0 );

	GLfloat tex[] = {0,0, 1,0, 1,1, 0,1};
	GLfloat box[] = {screen.width/2 - t.w/2, screen.height/2 - t.h,
					 screen.width/2 + t.w/2, screen.height/2 - t.h,
					 screen.width/2 + t.w/2, screen.height/2,
					 screen.width/2 - t.w/2, screen.height/2};

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 
	glVertexPointer(2, GL_FLOAT, 0,box);
	glTexCoordPointer(2, GL_FLOAT, 0, tex);
	glDrawArrays(GL_TRIANGLE_FAN,0,4);
	
	while (!q){
	SDL_PollEvent(&e);
	
		if (e.type == SDL_KEYDOWN)
			if (e.key.keysym.sym == layout[P1FIRE] || e.key.keysym.sym == layout[P2FIRE])
				q = 1;
		if (e.type == SDL_QUIT){
			glDeleteTextures(1, &t.t);
			close_sdl();
		}
		SDL_GL_SwapWindow(mywindow);
		SDL_Delay(50);
	}
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1, &t.t);
}

void collect_powerup(enemy_t *en, Player_t *p, int id){

	// 0 = bomb, 1 = dual bullets, 2 = kill enemy, 3 = life
	int type = powerups[id].type;
	play_sound(POWER, 0);
	float xdiff, ydiff, distance2, overlap;

	if (type == 0){
		// somehow explode all in certain radius
		for (int i = 0; i < no_of_asteroids; ++i){			// player - asteroid
		// compare ship
			xdiff =  p->sh.position.x - field[i].position.x;
			ydiff =  p->sh.position.y - field[i].position.y;
			distance2 = xdiff * xdiff + ydiff * ydiff;
			overlap = distance2 - field[i].size * field[i].size * 24 - 144 - 2 * field[i].size * 5 * 10;
			if (overlap <= 150000 * screen.obj_scale_factor){
				set_parts(field[i].position.x, field[i].position.y);
				play_sound(HIT, 0);
				memmove(&field[i], &field[no_of_asteroids-1], sizeof(asteroid_t));
				no_of_asteroids -= 1;
			}
		}
		
	}
	else if (type == 1){
		p->lives += 1;
	}
		else if (type == 2){
			for (int i = 0; i < no_of_enemy; ++i){
				set_enemy_parts(en[i].sh.position.x, en[i].sh.position.y);
			}

		play_sound(HIT, 0);
		no_of_enemy = 0;
	}
	else if (type == 3){
		p->sh.gun = 1;
	}

	memmove(&powerups[id], &powerups[no_of_powerups - 1], sizeof(powerup_t));
	no_of_powerups -= 1;
}
