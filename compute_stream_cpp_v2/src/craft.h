#ifndef _craft_h_
#define _craft_h_

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <curl/curl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "auth.h"
#include "client.h"
#include "config.h"
#include "craft.h"
#include "cube.h"
#include "db.h"
#include "item.h"
#include "map.h"
#include "matrix.h"
#include "noise.h"
#include "sign.h"
#include "tinycthread.h"
#include "util.h"
#include "world.h"

#define MAX_CHUNKS 8192
#define MAX_PLAYERS 128
#define WORKERS 4
#define MAX_TEXT_LENGTH 256
#define MAX_NAME_LENGTH 32
#define MAX_PATH_LENGTH 256
#define MAX_ADDR_LENGTH 256

#define ALIGN_LEFT 0
#define ALIGN_CENTER 1
#define ALIGN_RIGHT 2

#define MODE_OFFLINE 0
#define MODE_ONLINE 1

#define WORKER_IDLE 0
#define WORKER_BUSY 1
#define WORKER_DONE 2

typedef struct {
    Map map;
    Map lights;
    SignList signs;
    int p;
    int q;
    int faces;
    int sign_faces;
    int dirty;
    int miny;
    int maxy;
    GLuint buffer;
    GLuint sign_buffer;
} Chunk;

typedef struct {
    int p;
    int q;
    int load;
    Map *block_maps[3][3];
    Map *light_maps[3][3];
    int miny;
    int maxy;
    int faces;
    GLfloat *data;
} WorkerItem;

typedef struct {
    int index;
    int state;
    thrd_t thrd;
    mtx_t mtx;
    cnd_t cnd;
    WorkerItem item;
} Worker;

typedef struct {
    int x;
    int y;
    int z;
    int w;
} Block;

typedef struct {
    float x;
    float y;
    float z;
    float rx;
    float ry;
    float t;
} State;

typedef struct {
    int id;
    char name[MAX_NAME_LENGTH];
    State state;
    State state1;
    State state2;
    GLuint buffer;
} Player;

typedef struct {
    GLuint program;
    GLuint position;
    GLuint normal;
    GLuint uv;
    GLuint matrix;
    GLuint sampler;
    GLuint camera;
    GLuint timer;
    GLuint extra1;
    GLuint extra2;
    GLuint extra3;
    GLuint extra4;
    GLuint baseline;
} Attrib;

typedef struct {
    GLFWwindow *window;
    GLFWwindow *windows[4];
    Worker workers[WORKERS];
    Chunk chunks[MAX_CHUNKS];
    int chunk_count;
    int create_radius;
    int render_radius;
    int delete_radius;
    int sign_radius;
    Player players[MAX_PLAYERS];
    int player_count;
    int typing;
    char typing_buffer[MAX_TEXT_LENGTH];
    int message_index;
    char messages[MAX_MESSAGES][MAX_TEXT_LENGTH];
    int width;
    int height;
    int observe1;
    int observe2;
    int flying;
    int item_index;
    int scale;
    int ortho;
    float fov;
    int suppress_char;
    int mode;
    int mode_changed;
    char db_path[MAX_PATH_LENGTH];
    char server_addr[MAX_ADDR_LENGTH];
    int server_port;
    int day_length;
    int time_changed;
    Block block0;
    Block block1;
    Block copy0;
    Block copy1;
    int craft;
    int craft_changed;
    float baseline;
} Model;

extern Model model;
extern Model *g;

int chunked(float x);
float time_of_day();
int get_scale_factor();
GLuint gen_sky_buffer();
GLuint gen_player_buffer(float x, float y, float z, float rx, float ry);
void interpolate_player(Player *player);
void delete_all_players();
Player *player_crosshair(Player *player);
int highest_block(float x, float z);
void delete_chunks();
void delete_all_chunks();
void force_chunks(Player *player);
int worker_run(void *arg);
int render_chunks(Attrib *attrib, Player *player);
void render_signs(Attrib *attrib, Player *player);
void render_sign(Attrib *attrib, Player *player);
void render_players(Attrib *attrib, Player *player);
void render_sky(Attrib *attrib, Player *player, GLuint buffer);
void render_wireframe(Attrib *attrib, Player *player);
void render_crosshairs(Attrib *attrib);
void render_item(Attrib *attrib);
void render_text(Attrib *attrib, int justify, float x, float y, float n, char *text);
void login();
void on_key(GLFWwindow *window, int key, int scancode, int action, int mods);
void on_char(GLFWwindow *window, unsigned int u);
void on_scroll(GLFWwindow *window, double xdelta, double ydelta);
void on_mouse_button(GLFWwindow *window, int button, int action, int mods);
void handle_mouse_input();
void handle_movement(double dt);
void parse_buffer(char *buffer);
void reset_model();

#endif
