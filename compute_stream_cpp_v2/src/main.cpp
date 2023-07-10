#define GL_SILENCE_DEPRECATION
#define GLFW_INCLUDE_GLCOREARB
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <curl/curl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <iostream>
#include <fstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

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
}

#define BASELINE 0.5f
#define WIDTH 800
#define HEIGHT 800

class AVVideo {
    public:
        AVVideo(char *filename);
        ~AVVideo();

        void getFrame();

    private:
        AVFormatContext     *pFormatCtx = NULL;
        int                 i, videoStream;
        AVCodecContext      *pCodecCtx = NULL;
        AVCodec             *pCodec = NULL;
        AVFrame             *pFrame = NULL; 
        AVFrame             *pFrameRGB = NULL;
        AVPacket            packet;
        int                 frameFinished;
        int                 numBytes;
        uint8_t             *buffer = NULL;

        AVDictionary        *optionsDict = NULL;
        struct SwsContext   *sws_ctx = NULL;

        int error;
};

AVVideo::AVVideo(char *filename) {
    error = 0;

    // Register all formats and codecs
    av_register_all();
    
    // Open video file
    if (avformat_open_input(&pFormatCtx, filename, NULL, NULL) != 0) {
        error = -1;
        return; // Couldn't open file
    }
    
    // Retrieve stream information
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        error = -1;
        return; // Couldn't find stream information
    }
    
    // Dump information about file onto standard error
    av_dump_format(pFormatCtx, 0, filename, 0);
    
    // Find the first video stream
    videoStream = -1;
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if(pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            break;
        }
    }
    if (videoStream == -1) {
        error = -1;
        return; // Didn't find a video stream
    }
    
    // Get a pointer to the codec context for the video stream
    pCodecCtx = pFormatCtx->streams[videoStream]->codec;
    
    // Find the decoder for the video stream
    pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (pCodec == NULL) {
        fprintf(stderr, "Unsupported codec!\n");
        error = -1;
        return; // Codec not found
    }
    // Open codec
    if (avcodec_open2(pCodecCtx, pCodec, &optionsDict) < 0) {
        error = -1;
        return; // Could not open codec
    }
    
    // Allocate video frame
    pFrame = av_frame_alloc();
    
    // Allocate an AVFrame structure
    pFrameRGB = av_frame_alloc();
    if (pFrameRGB == NULL) {
        error = -1;
        return;
    }
    
    // Determine required buffer size and allocate buffer
    numBytes = avpicture_get_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height);
    buffer = (uint8_t *) av_malloc(numBytes * sizeof(uint8_t));

    sws_ctx =
        sws_getContext
        (
            pCodecCtx->width,
            pCodecCtx->height,
            pCodecCtx->pix_fmt,
            pCodecCtx->width,
            pCodecCtx->height,
            AV_PIX_FMT_RGB24,
            SWS_BILINEAR,
            NULL,
            NULL,
            NULL
        );
    
    // Assign appropriate parts of buffer to image planes in pFrameRGB
    // Note that pFrameRGB is an AVFrame, but AVFrame is a superset
    // of AVPicture
    avpicture_fill((AVPicture *) pFrameRGB, buffer, AV_PIX_FMT_RGB24,
            pCodecCtx->width, pCodecCtx->height);
}

AVVideo::~AVVideo()
{
    // Free the RGB image
    av_free(buffer);
    av_free(pFrameRGB);
    
    // Free the YUV frame
    av_free(pFrame);
    
    // Close the codec
    avcodec_close(pCodecCtx);
    
    // Close the video file
    avformat_close_input(&pFormatCtx);
}

void AVVideo::getFrame()
{
    int val = av_read_frame(pFormatCtx, &packet);

    // Loop if at end of file
    if (val < 0) {
        av_seek_frame(pFormatCtx, -1, 0, AVSEEK_FLAG_FRAME);
        val = av_read_frame(pFormatCtx, &packet);
    }

    if (val >= 0) {
        // Is this a packet from the video stream?
        if (packet.stream_index == videoStream) {
            // Decode video frame
            avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, &packet);
            
            // Did we get a video frame?
            if (frameFinished) {
                // Convert the image from its native format to RGB
                sws_scale
                (
                    sws_ctx,
                    (uint8_t const * const *) pFrame->data,
                    pFrame->linesize,
                    0,
                    pCodecCtx->height,
                    pFrameRGB->data,
                    pFrameRGB->linesize
                );
                
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, pCodecCtx->width, pCodecCtx->height, 0, GL_RGB, GL_UNSIGNED_BYTE, pFrameRGB->data[0]);

                // Free the packet that was allocated by av_read_frame
                av_free_packet(&packet);

                return;
            }
        }
        
        // Free the packet that was allocated by av_read_frame
        av_free_packet(&packet);
    }
        
    return;
}

void create_window() {
    int window_width = WINDOW_WIDTH;
    int window_height = WINDOW_HEIGHT;
    GLFWmonitor *monitor = NULL;
    if (FULLSCREEN) {
        int mode_count;
        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *modes = glfwGetVideoModes(monitor, &mode_count);
        window_width = modes[mode_count - 1].width;
        window_height = modes[mode_count - 1].height;
    }

    glfwWindowHint(GLFW_DECORATED, 0);

    g->windows[0] = glfwCreateWindow(
        WIDTH/2, HEIGHT/2, "Craft (left color)", monitor, NULL);
    glfwSetWindowPos(g->windows[0], 0, 0);

    g->windows[1] = glfwCreateWindow(
        WIDTH/2, HEIGHT/2, "Craft (left depth)", monitor, g->windows[0]);
    glfwSetWindowPos(g->windows[1], 0, 500);

    g->windows[2] = glfwCreateWindow(
        WIDTH/2, HEIGHT/2, "Craft (right color)", monitor, g->windows[0]);
    glfwSetWindowPos(g->windows[2], 500, 0);

    g->windows[3] = glfwCreateWindow(
        WIDTH/2, HEIGHT/2, "Craft (right depth)", monitor, g->windows[0]);
    glfwSetWindowPos(g->windows[3], 500, 500);

    g->window = g->windows[0];
    glfwFocusWindow(g->window);
}

void render_quad()
{
    glBegin(GL_TRIANGLE_FAN);

    if (!g->craft) {
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);

        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, -1.0f);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
    } else { // Flip texture coordinates vertically for videos
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 1.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, -1.0f);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, -1.0f);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);
    }

    glEnd();
}

void read_homography(char *filename, float *matrix)
{
    std::fstream file(filename, std::ios_base::in);

    for (int k = 0; k < 9; k++) {
        file >> matrix[k];
    }
}

int main(int argc, char **argv) {
    // INITIALIZATION //
    curl_global_init(CURL_GLOBAL_DEFAULT);
    srand(time(NULL));
    rand();

    g->craft = 0;
    g->craft_changed = 0;
    g->baseline = 0;

    float homo1[9], homo2[9], homo3[9], homo4[9];
    AVVideo *vid1 = NULL, *vid2 = NULL, *vid3 = NULL, *vid4 = NULL;

    read_homography("./homography/left_color.txt", homo1);
    read_homography("./homography/left_depth.txt", homo2);
    read_homography("./homography/right_color.txt", homo3);
    read_homography("./homography/right_depth.txt", homo4);

    // WINDOW INITIALIZATION //
    if (!glfwInit()) {
        return -1;
    }

    create_window();
    if (!g->window) {
        glfwTerminate();
        return -1;
    }

    unsigned int major = glfwGetWindowAttrib(g->window, GLFW_CONTEXT_VERSION_MAJOR);
    unsigned int minor = glfwGetWindowAttrib(g->window, GLFW_CONTEXT_VERSION_MINOR);
    printf("%d %d\n", major, minor);

    glfwMakeContextCurrent(g->window);

    if (glewInit() != GLEW_OK) {
        return -1;
    }

    glfwSwapInterval(VSYNC);
    glfwSetInputMode(g->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetKeyCallback(g->window, on_key);
    glfwSetCharCallback(g->window, on_char);
    glfwSetMouseButtonCallback(g->window, on_mouse_button);
    glfwSetScrollCallback(g->window, on_scroll);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glLogicOp(GL_INVERT);
    glClearColor(0, 0, 0, 1);


    // CREATE FRAMEBUFFER //
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    // LOAD TEXTURES //
    GLuint texture;
    glGenTextures(1, &texture);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/texture.png");

    GLuint font;
    glGenTextures(1, &font);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, font);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    load_png_texture("textures/font.png");

    GLuint sky;
    glGenTextures(1, &sky);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, sky);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    load_png_texture("textures/sky.png");

    GLuint sign;
    glGenTextures(1, &sign);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, sign);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    load_png_texture("textures/sign.png");

    glActiveTexture(GL_TEXTURE4);

    GLuint fbo_color;
    glGenTextures(1, &fbo_color);
    glBindTexture(GL_TEXTURE_2D, fbo_color);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_color, 0);

    GLuint fbo_depth;
    glGenTextures(1, &fbo_depth);
    glBindTexture(GL_TEXTURE_2D, fbo_depth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, WIDTH, HEIGHT, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fbo_depth, 0);

    GLuint frame;
    glGenTextures(1, &frame);
    glBindTexture(GL_TEXTURE_2D, frame);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    // LOAD SHADERS //
    Attrib block_attrib = {0};
    Attrib line_attrib = {0};
    Attrib text_attrib = {0};
    Attrib sky_attrib = {0};
    Attrib color_attrib = {0};
    Attrib depth_attrib = {0};
    GLuint program;

    program = load_program(
        "shaders/block_vertex.glsl", "shaders/block_fragment.glsl");
    block_attrib.program = program;
    block_attrib.position = glGetAttribLocation(program, "position");
    block_attrib.normal = glGetAttribLocation(program, "normal");
    block_attrib.uv = glGetAttribLocation(program, "uv");
    block_attrib.matrix = glGetUniformLocation(program, "matrix");
    block_attrib.sampler = glGetUniformLocation(program, "sampler");
    block_attrib.extra1 = glGetUniformLocation(program, "sky_sampler");
    block_attrib.extra2 = glGetUniformLocation(program, "daylight");
    block_attrib.extra3 = glGetUniformLocation(program, "fog_distance");
    block_attrib.extra4 = glGetUniformLocation(program, "ortho");
    block_attrib.camera = glGetUniformLocation(program, "camera");
    block_attrib.timer = glGetUniformLocation(program, "timer");
    block_attrib.baseline = glGetUniformLocation(program, "baseline");

    program = load_program(
        "shaders/line_vertex.glsl", "shaders/line_fragment.glsl");
    line_attrib.program = program;
    line_attrib.position = glGetAttribLocation(program, "position");
    line_attrib.matrix = glGetUniformLocation(program, "matrix");
    line_attrib.baseline = glGetUniformLocation(program, "baseline");

    program = load_program(
        "shaders/text_vertex.glsl", "shaders/text_fragment.glsl");
    text_attrib.program = program;
    text_attrib.position = glGetAttribLocation(program, "position");
    text_attrib.uv = glGetAttribLocation(program, "uv");
    text_attrib.matrix = glGetUniformLocation(program, "matrix");
    text_attrib.sampler = glGetUniformLocation(program, "sampler");
    text_attrib.extra1 = glGetUniformLocation(program, "is_sign");

    program = load_program(
        "shaders/sky_vertex.glsl", "shaders/sky_fragment.glsl");
    sky_attrib.program = program;
    sky_attrib.position = glGetAttribLocation(program, "position");
    sky_attrib.normal = glGetAttribLocation(program, "normal");
    sky_attrib.uv = glGetAttribLocation(program, "uv");
    sky_attrib.matrix = glGetUniformLocation(program, "matrix");
    sky_attrib.sampler = glGetUniformLocation(program, "sampler");
    sky_attrib.timer = glGetUniformLocation(program, "timer");

    program = load_program(
        "shaders/color_vertex.glsl", "shaders/color_fragment.glsl");
    color_attrib.program = program;
    color_attrib.matrix = glGetUniformLocation(program, "matrix");
    color_attrib.sampler = glGetUniformLocation(program, "sampler");

    program = load_program(
        "shaders/depth_vertex.glsl", "shaders/depth_fragment.glsl");
    depth_attrib.program = program;
    depth_attrib.matrix = glGetUniformLocation(program, "matrix");
    depth_attrib.sampler = glGetUniformLocation(program, "sampler");


    // CHECK COMMAND LINE ARGUMENTS //
    if (argc == 2 || argc == 3) {
        g->mode = MODE_ONLINE;
        strncpy(g->server_addr, argv[1], MAX_ADDR_LENGTH);
        g->server_port = argc == 3 ? atoi(argv[2]) : DEFAULT_PORT;
        snprintf(g->db_path, MAX_PATH_LENGTH,
            "cache.%s.%d.db", g->server_addr, g->server_port);
    }
    else {
        g->mode = MODE_OFFLINE;
        snprintf(g->db_path, MAX_PATH_LENGTH, "%s", DB_PATH);
    }

    g->create_radius = CREATE_CHUNK_RADIUS;
    g->render_radius = RENDER_CHUNK_RADIUS;
    g->delete_radius = DELETE_CHUNK_RADIUS;
    g->sign_radius = RENDER_SIGN_RADIUS;

    // INITIALIZE WORKER THREADS
    for (int i = 0; i < WORKERS; i++) {
        Worker *worker = g->workers + i;
        worker->index = i;
        worker->state = WORKER_IDLE;
        mtx_init(&worker->mtx, mtx_plain);
        cnd_init(&worker->cnd);
        thrd_create(&worker->thrd, worker_run, worker);
    }

    // OUTER LOOP //
    int running = 1;
    while (running) {
        // DATABASE INITIALIZATION //
        if (g->mode == MODE_OFFLINE || USE_CACHE) {
            db_enable();
            if (db_init(g->db_path)) {
                return -1;
            }
            if (g->mode == MODE_ONLINE) {
                // TODO: support proper caching of signs (handle deletions)
                db_delete_all_signs();
            }
        }

        // CLIENT INITIALIZATION //
        if (g->mode == MODE_ONLINE) {
            client_enable();
            client_connect(g->server_addr, g->server_port);
            client_start();
            client_version(1);
            login();
        }

        // LOCAL VARIABLES //
        reset_model();
        FPS fps = {0, 0, 0};
        double last_commit = glfwGetTime();
        double last_update = glfwGetTime();
        GLuint sky_buffer = gen_sky_buffer();

        Player *me = g->players;
        State *s = &g->players->state;
        me->id = 0;
        me->name[0] = '\0';
        me->buffer = 0;
        g->player_count = 1;

        // LOAD STATE FROM DATABASE //
        int loaded = db_load_state(&s->x, &s->y, &s->z, &s->rx, &s->ry);
        force_chunks(me);
        if (!loaded) {
            s->y = highest_block(s->x, s->z) + 2;
        }

        // BEGIN MAIN LOOP //
        double previous = glfwGetTime();
        while (1) {
            if (g->craft_changed) {
                if (vid1) delete vid1;
                if (vid2) delete vid2;
                if (vid3) delete vid3;
                if (vid4) delete vid4;

                vid1 = NULL;
                vid2 = NULL;
                vid3 = NULL;
                vid4 = NULL;

                if (g->craft) {
                    char buffer[256];

                    snprintf(buffer, 256, "./scenes/%d_left_color.mp4", g->craft);
                    printf(buffer);
                    vid1 = new AVVideo(buffer);

                    snprintf(buffer, 256, "./scenes/%d_left_depth.mp4", g->craft);
                    vid2 = new AVVideo(buffer);

                    snprintf(buffer, 256, "./scenes/%d_right_color.mp4", g->craft);
                    vid3 = new AVVideo(buffer);

                    snprintf(buffer, 256, "./scenes/%d_right_depth.mp4", g->craft);
                    vid4 = new AVVideo(buffer);
                }

                g->craft_changed = 0;
            }

            // FRAME RATE //
            if (g->time_changed) {
                g->time_changed = 0;
                last_commit = glfwGetTime();
                last_update = glfwGetTime();
                memset(&fps, 0, sizeof(fps));
            }
            update_fps(&fps);
            printf("FPS: %d\n", fps.fps);
            double now = glfwGetTime();
            double dt = now - previous;
            dt = MIN(dt, 0.2);
            dt = MAX(dt, 0.0);
            previous = now;

            // HANDLE MOUSE INPUT //
            handle_mouse_input();

            // HANDLE MOVEMENT //
            handle_movement(dt);

            // HANDLE DATA FROM SERVER //
            char *buffer = client_recv();
            if (buffer) {
                parse_buffer(buffer);
                free(buffer);
            }

            // FLUSH DATABASE //
            if (now - last_commit > COMMIT_INTERVAL) {
                last_commit = now;
                db_commit();
            }

            // SEND POSITION TO SERVER //
            if (now - last_update > 0.1) {
                last_update = now;
                client_position(s->x, s->y, s->z, s->rx, s->ry);
            }

            // PREPARE TO RENDER //
            g->observe1 = g->observe1 % g->player_count;
            g->observe2 = g->observe2 % g->player_count;
            delete_chunks();
            del_buffer(me->buffer);
            me->buffer = gen_player_buffer(s->x, s->y, s->z, s->rx, s->ry);
            for (int i = 1; i < g->player_count; i++) {
                interpolate_player(g->players + i);
            }
            Player *player = g->players + g->observe1;


            if (!g->craft) {
                g->baseline = BASELINE;

                // RENDER 3-D SCENE //
                glfwMakeContextCurrent(g->windows[0]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);

                g->scale = get_scale_factor();
                glfwGetFramebufferSize(g->windows[0], &g->width, &g->height);
                glViewport(0, 0, g->width, g->height);

                glClear(GL_COLOR_BUFFER_BIT);
                glClear(GL_DEPTH_BUFFER_BIT);
                render_sky(&sky_attrib, player, sky_buffer);
                glClear(GL_DEPTH_BUFFER_BIT);
                int face_count = render_chunks(&block_attrib, player);
                render_signs(&text_attrib, player);
                render_sign(&text_attrib, player);
                render_players(&block_attrib, player);
                if (SHOW_WIREFRAME) {
                    render_wireframe(&line_attrib, player);
                }

                glUseProgram(0);

                // SWAP AND POLL //
                glfwSwapBuffers(g->windows[0]);
            }


            // START RENDER FRAMEBUFFER //
            glfwMakeContextCurrent(g->windows[0]);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glUseProgram(color_attrib.program);
            glUniformMatrix3fv(color_attrib.matrix, 1, GL_FALSE, homo1);
            glUniform1i(color_attrib.sampler, 0);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            if (!g->craft) {
                glBindTexture(GL_TEXTURE_2D, fbo_color);
            } else {
                glBindTexture(GL_TEXTURE_2D, frame);
                vid1->getFrame();
            }
            render_quad();

            glfwSwapBuffers(g->windows[0]);


            // START RENDER FRAMEBUFFER //
            glfwMakeContextCurrent(g->windows[1]);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glUseProgram(depth_attrib.program);
            glUniformMatrix3fv(depth_attrib.matrix, 1, GL_FALSE, homo2);
            glUniform1i(depth_attrib.sampler, 0);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            if (!g->craft) {
                glBindTexture(GL_TEXTURE_2D, fbo_depth);
            } else {
                glBindTexture(GL_TEXTURE_2D, frame);
                vid2->getFrame();
            }
            render_quad();

            glfwSwapBuffers(g->windows[1]);


            if (!g->craft) {
                g->baseline = -BASELINE;

                // RENDER 3-D SCENE //
                glfwMakeContextCurrent(g->windows[0]);
                glBindFramebuffer(GL_FRAMEBUFFER, fbo);

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);

                g->scale = get_scale_factor();
                glfwGetFramebufferSize(g->windows[0], &g->width, &g->height);
                glViewport(0, 0, g->width, g->height);

                glClear(GL_COLOR_BUFFER_BIT);
                glClear(GL_DEPTH_BUFFER_BIT);
                render_sky(&sky_attrib, player, sky_buffer);
                glClear(GL_DEPTH_BUFFER_BIT);
                int face_count = render_chunks(&block_attrib, player);
                render_signs(&text_attrib, player);
                render_sign(&text_attrib, player);
                render_players(&block_attrib, player);
                if (SHOW_WIREFRAME) {
                    render_wireframe(&line_attrib, player);
                }

                glUseProgram(0);

                // SWAP AND POLL //
                glfwSwapBuffers(g->windows[0]);
            }


            // START RENDER FRAMEBUFFER //
            glfwMakeContextCurrent(g->windows[2]);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glUseProgram(color_attrib.program);
            glUniformMatrix3fv(color_attrib.matrix, 1, GL_FALSE, homo3);
            glUniform1i(color_attrib.sampler, 0);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            if (!g->craft) {
                glBindTexture(GL_TEXTURE_2D, fbo_color);
            } else {
                glBindTexture(GL_TEXTURE_2D, frame);
                vid3->getFrame();
            }
            render_quad();

            glfwSwapBuffers(g->windows[2]);


            // START RENDER FRAMEBUFFER //
            glfwMakeContextCurrent(g->windows[3]);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glUseProgram(depth_attrib.program);
            glUniformMatrix3fv(depth_attrib.matrix, 1, GL_FALSE, homo4);
            glUniform1i(depth_attrib.sampler, 0);

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glEnable(GL_TEXTURE_2D);
            if (!g->craft) {
                glBindTexture(GL_TEXTURE_2D, fbo_depth);
            } else {
                glBindTexture(GL_TEXTURE_2D, frame);
                vid4->getFrame();
            }
            render_quad();

            glfwSwapBuffers(g->windows[3]);


            glfwPollEvents();
            if (glfwWindowShouldClose(g->windows[0])) {
                running = 0;
                break;
            }
            if (g->mode_changed) {
                g->mode_changed = 0;
                break;
            }
        }

        // SHUTDOWN //
        db_save_state(s->x, s->y, s->z, s->rx, s->ry);
        db_close();
        db_disable();
        client_stop();
        client_disable();
        del_buffer(sky_buffer);
        delete_all_chunks();
        delete_all_players();
    }

    glfwTerminate();
    curl_global_cleanup();
    return 0;
}
