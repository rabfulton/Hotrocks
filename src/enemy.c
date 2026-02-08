
#include "main.h"
#define CRITICAL_DIST 250
#define MAX_SPEED 10

extern screen_t screen;

extern int no_of_enemy;
extern int no_of_asteroids;

enum{ 	no_plan,
		plan,
		avoidsh,
		getplay
		};
		
 
Vector mass_center(enemy_t *en);
void near_asteroid(enemy_t *en, asteroid_t *field);
void near_ship(enemy_t *en);

void plan_avoid_obstacle(enemy_t *en, Vector *obs, float mag);
void plan_go_avg(enemy_t *en, Vector v);
void plan_get_player(ship_t *sh, enemy_t *en);
void do_plan(enemy_t *en);
//void keywait();
void shoot_player(enemy_t *en);
static int enemy_activate_shield(enemy_t *en);

void create_enemies(enemy_t *en, int num){

	int i = no_of_enemy;	// number of enemies alive
	if(i == MAX_ENEMY)
		return;
	
	en[i].sh.position.x = screen.width / (1 + i);
	en[i].sh.position.y = 0;
	en[i].sh.velocity.x = 0;
	en[i].sh.velocity.y = 0;
	en[i].sh.angle = i;
	en[i].sh.shield_power = 0;
	en[i].sh.shield_timer = 0;
	en[i].sh.shield = 0;
	en[i].sh.gun = 0;
	en[i].plan = 0;
	en[i].target_mag = 0.1;
	en[i].target = 0;
	en[i].skill = 2;
	en[i].shield_uses = 3;
	en[i].shield_until = 0;

	for (int j = 0; j < MAX_BULLETS; ++j){
		en[i].bulls[j].position.x = -99999;
		en[i].bulls[j].position.y = -99999;
	}

	no_of_enemy += 1;

}
	
void move_enemies(enemy_t *en, asteroid_t *field, Player_t *p, int mode){

	near_ship(en);
	near_asteroid(en, field);
			
	for (int i = 0; i < no_of_enemy; ++i){
		if (en[i].sh.shield && SDL_GetTicks() >= en[i].shield_until){
			en[i].sh.shield = 0;
		}

		if (en[i].sh.position.x > screen.width + 12)
			en[i].sh.position.x = -12;
		
		if (en[i].sh.position.x < -12)
			en[i].sh.position.x = screen.width + 12;
		
		if (en[i].sh.position.y > screen.height + 12)
			en[i].sh.position.y = -12;
		
		if (en[i].sh.position.y < -12)
			en[i].sh.position.y = screen.height + 12;
			
		en[i].sh.position.x += en[i].sh.velocity.x;
		en[i].sh.position.y += en[i].sh.velocity.y;
		
		if (en[i].sh.velocity.x > 2)
			en[i].sh.velocity.x -= 0.2;
		if (en[i].sh.velocity.y > 2)
			en[i].sh.velocity.y -= 0.2;
		if (en[i].sh.velocity.y < -2)
			en[i].sh.velocity.y += 0.2;
		if (en[i].sh.velocity.x < -2)
			en[i].sh.velocity.x += 0.2;			

		if (en[i].plan > 0){
			do_plan(&en[i]);
		}
		if (en[i].plan == 0 && mode == 0){
			plan_get_player(&p[0].sh, &en[i]);	
		}
		if (mode > 0 && en[i].plan == 0){
				plan_get_player(&p[i%2].sh, &en[i]);
		}
	}
}

Vector mass_center(enemy_t *en){

	Vector v;
	v.x = 0;
	v.y = 0;
	
	for (int i = 0; i < no_of_enemy; ++i){
		v.x += en[i].sh.position.x;
		v.y += en[i].sh.position.y;
	}
	v.x /= no_of_enemy;
	v.y /= no_of_enemy;
	
	return v;
}

void near_ship(enemy_t *en){

	float xdiff, ydiff;
	float distance, overlap;

	for (int i = 0; i < no_of_enemy - 1; ++i){		// each enemy.....

	
		for (int j = i + 1; j < no_of_enemy; ++j){	// compare to each successive enemy
			xdiff =  en[i].sh.position.x - en[j].sh.position.x;
			ydiff =  en[i].sh.position.y - en[j].sh.position.y;
			distance = sqrt(xdiff * xdiff + ydiff * ydiff);		// sqrt not needed
			overlap = distance - 24;
			
			if (overlap <= 160){

				if (en[i].plan != avoidsh){
					plan_avoid_obstacle(&en[i], &en[j].sh.position, overlap);
					en[i].plan = avoidsh;
				}
				if (en[j].plan != avoidsh){		
					plan_avoid_obstacle(&en[j], &en[i].sh.position, overlap);
					en[j].plan = avoidsh;
				}
			}
		}
	}
}	

void near_asteroid(enemy_t *en, asteroid_t *field){

	// test if close to asteroid or dead
	// if too close to asteroid then break and
	// avoiding asteroid is top priority.
	
	float xdiff, ydiff;
	float distance, smallest;
	int s = 0;
	int dead = 0;
	const float asteroid_radius = 4.5f;
	
	for (int j = 0; j < no_of_enemy; ++j){
		smallest = 99999;
		for (int i = 0; i < no_of_asteroids; ++i){
				
			xdiff =  en[j].sh.position.x - field[i].position.x;
			ydiff =  en[j].sh.position.y - field[i].position.y;
			distance = sqrt(xdiff * xdiff + ydiff * ydiff);
			float ai_overlap = distance - field[i].size * 5 * screen.obj_scale_factor - 12 * screen.obj_scale_factor;
			float distance2 = xdiff * xdiff + ydiff * ydiff;
			float player_style_overlap = distance2
				- field[i].size * field[i].size * asteroid_radius * asteroid_radius
				- 144.0f
				- 2.0f * field[i].size * asteroid_radius * 10.0f;
			if (smallest > distance){
				smallest = distance;
				s = i;
			}
			// If an asteroid is close and approaching, enemy may spend a shield burst.
			if (en[j].sh.shield == 0 && en[j].shield_uses > 0){
				float relx = field[i].position.x - en[j].sh.position.x;
				float rely = field[i].position.y - en[j].sh.position.y;
				float relvx = field[i].velocity.x - en[j].sh.velocity.x;
				float relvy = field[i].velocity.y - en[j].sh.velocity.y;
				float closing = relx * relvx + rely * relvy;
				// Last-resort trigger: very close and still closing.
				if (ai_overlap < 20.0f && closing < -5.0f){
					enemy_activate_shield(&en[j]);
				}
			}
			// Actual collision uses same overlap logic players use.
			if (player_style_overlap < 0){
				if (en[j].sh.shield || enemy_activate_shield(&en[j])){
					collide2(&field[i].velocity, &field[i].position, field[i].size, &en[j].sh.velocity, &en[j].sh.position, 1);
					en[j].plan = avoidsh;
				}
				else{
					dead = 1;
				}
				break;
			}
		}
		if (dead){
			kill_enemy(en, j);
			if (j != no_of_enemy - 1){
				--j;
			}
			dead = 0;
		}
		else if (smallest < CRITICAL_DIST && en[j].plan != avoidsh){
			plan_avoid_obstacle(&en[j], &field[s].position, smallest);
		}
	}
}

void kill_enemy(enemy_t *en, int id){

	play_sound(HIT, 0);
	set_enemy_parts(en[id].sh.position.x, en[id].sh.position.y);
	
	memmove(&en[id], &en[no_of_enemy-1], sizeof(enemy_t));
	no_of_enemy -= 1;
}

void plan_avoid_obstacle(enemy_t *en, Vector *obs, float mag){
	
	double xdiff, ydiff;

	xdiff =  en->sh.position.x - obs->x;
	ydiff =  en->sh.position.y - obs->y;
	en->target_mag = (300-mag) / 1000;
	
	en->target = atan2(ydiff, xdiff);
	en->plan = 1;
}

void plan_go_avg(enemy_t *en, Vector v){

	float x, y;
	// turn towards mass center and thrust
	x = en->sh.position.x - v.x;
	y = en->sh.position.y - v.y;
	//~ if (en->position.x > v.x){
		//~ x = -x;
		//~ y = -y;
	//~ }
	//~ 
	en->target = atan2(y, x);
	en->plan = 1;
}

void plan_get_player(ship_t *sh, enemy_t *en){

	float x, y, d;

	x = sh->position.x - en->sh.position.x;
	y = sh->position.y - en->sh.position.y;

	d = sqrt(x*x + y*y);

	if (d > screen.width/5){
		en->target = atan2f(y, x);
		en->plan = getplay;
	}
	else{
		en->target = atan2f(-y, -x);
		en->plan = avoidsh;
	}
}

void do_plan(enemy_t *en){

	// turn towards target vector
	// if at target vector & below top speed - thrust, plan = 0.
	if ((en->sh.angle > en->target - 0.1) && (en->sh.angle < en->target + 0.1)){
		en->sh.velocity.x += en->target_mag * cos(en->sh.angle);
		en->sh.velocity.y += en->target_mag * sin(en->sh.angle);

		en->target_mag = 0.1;
		init_thruster(&en->sh);
		if (en->plan == getplay){
			shoot_player(en);
		}
		en->plan = 0;
		return;
	}
	else if (en->sh.angle < en->target)
		en->sh.angle += 0.09;
	else
		en->sh.angle -= 0.09;
	
}

void shoot_player(enemy_t *en){

	float x = 100 * ((float)rand()/(float)RAND_MAX);
	
	if (x > 100 - en->skill){
		create_bullet_rel(en->bulls, &en->sh);
	}
}

static int enemy_activate_shield(enemy_t *en){

	if (en->sh.shield || en->shield_uses <= 0)
		return 0;

	en->sh.shield = 1;
	en->shield_uses -= 1;
	en->shield_until = SDL_GetTicks() + 4000;
	return 1;
}

//void keywait(){

	//SDL_Event e;
	
	//while (1){
		//SDL_PollEvent(&e);
		//if(e.type == SDL_KEYDOWN)
			//return;
		//SDL_Delay(50);
	//}
//}
