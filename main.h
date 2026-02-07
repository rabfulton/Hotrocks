#include <GL/glew.h>
#include <SDL_mixer.h>
#include <libopenmpt/libopenmpt.h>
#include <time.h>
#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdio.h>
#include <math.h>
#include <SDL_opengl.h>
//#include <unistd.h>
#define PI 3.141592653
#define MAX_BULLETS 32
#define MAX_ASTEROIDS 256
#define MAX_ENEMY 16
#define DELTA_P 12				// Physics update interval in milliseconds
#define MAX_POWERUPS 3
#define POWERUP_PROBABILITY 0.1f
#define ASTEROID_VERTS 22

enum{
	CRASH,
	THRUST,
	FIRE,
	HIT,
	SHIELD,
	IMPACT,
	SELECTS,
	BOUNCE,
	HYPER,
	ENTRY,
	POWER,
	MAX_SOUNDS
};

enum{
	P1LEFT,
	P1RIGHT,
	P1THRUST,
	P1FIRE,
	P1SHIELD,
	P2LEFT,
	P2RIGHT,
	P2THRUST,
	P2FIRE,
	P2SHIELD,
	MOUSE_ENABLE
};

typedef struct Sound{
	Mix_Chunk *effect;
}Sound;

typedef struct {float x; float y;}Vector;

typedef struct {float x; float y;}Camera;

typedef struct {Vector position;
				int type;
				int age;
				}powerup_t;

typedef struct {Vector velocity;
				Vector position;
				float age;
				int state;
				}particle_t;
				
typedef struct {
				Vector position;		// current position
				float angle;			// current angle of rotation in radians
				float angle_inc;
				Vector velocity;	
				float size;				// size of asteroid
				int has_collided;
			}asteroid_t;

typedef struct {
				Vector position;
 				Vector velocity;
			}bullet_t;
			
typedef struct ship_t{
				particle_t thrust[12];
				Vector position;		
				float angle;
				Vector velocity;				
				float shield_power;
				int shield_timer;
				int shield;
				Uint32 auto_shield_until;
				int gun;
				void (*fire)(bullet_t *, struct ship_t *);
			}ship_t;
			

typedef struct {
				int w;
				int h;
				GLuint t;
			}texture_t;

typedef struct {ship_t sh;
				bullet_t bulls[MAX_BULLETS];
				int lives;
				int score;
					}Player_t;
					
typedef struct{	ship_t sh;
				bullet_t bulls[MAX_BULLETS];
				int plan;
				float target;
				float target_mag;
				int skill;
				}enemy_t;

typedef struct{ float age;
				float x;
				float y;
				}explosion_t;

typedef struct{	float width;
				float height;
				float h_dpi;
				float v_dpi;
				float d_dpi;
				float obj_scale_factor;
				float ctl_scale_factor;
				}screen_t;
								
extern Sound sound[MAX_SOUNDS];				// Array for sounds index				

extern int layout[11];							// Array for current keymap
extern int run_state;
extern int SFX_VOLUME;
extern int MUSIC_VOLUME;
extern int FULLSCREEN;
extern int WINDOW_WIDTH;
extern int WINDOW_HEIGHT;
int init_sdl();

void initGL();
void close_sdl();
void init_sound();
void update_music_player(void);
void shutdown_music_player(void);
void set_sfx_volume(int volume);
void set_music_volume(int volume);

int main_menu(GLuint tex);
// INITS
void load_keys(int);
void init_player(Player_t *p, int id, int num);
void init_particles();
void init_thruster(ship_t *sh);
void init_level(asteroid_t *field, Player_t *p, enemy_t *en, int level, int mode);
void init_asteroid(asteroid_t *a, float size);
void create_enemies(enemy_t *en, int num);
void toggle_anti_aliasing(int);
void reload_context(void);
void set_fullscreen(int enabled);
void set_window_resolution(int width, int height);
void allocate_memory(void);
// EVENTS
void set_parts(int x, int y);
void create_bullet(bullet_t *, ship_t *);
void create_bullet_rel(bullet_t *bulls, ship_t *sh);
void split_asteroid(asteroid_t *field, int a);
void collision_detect(asteroid_t *field, Player_t *, enemy_t *en);
void dual_collisions(Player_t *, Player_t *);
void play_sound(int index, int loop);
void kill_enemy(enemy_t *en, int id);
// MOVE
void move_asteroid(asteroid_t *field);
void move_ship(ship_t *sh);
void move_bullets(bullet_t *bulls);
void move_enemies(enemy_t *en, asteroid_t *field, Player_t *p, int mode);
void move_particles(void);
// RENDER
void draw_asteroids(asteroid_t *field);
void draw_ship(ship_t *sh, int type);
void draw_shield(ship_t *sh);
void draw_bullets(bullet_t *bulls);
void draw_scorebar(Player_t *, int mode);
void draw_background(GLuint);
void draw_particles(void);
void draw_powerup(powerup_t *p);
//void draw_enemies(enemy_t *en, int num); defunct
void draw_explosion(explosion_t *e);
void draw_menu_item(int x, int y, char text[], int colour, int size);
void set_audio(GLuint tex);
void set_resolution(GLuint tex);
void high_scores(int p1score, int p2score, GLuint tex);

GLuint load_texture(const char *s);
void render_text(texture_t *t, char s[], int c, int size);

void level_transition(int level);
int keyboard_events(Player_t *p1, Player_t *p2, int mode);
int power_two(int);
