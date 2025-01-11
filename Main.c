#include "external/include/SDL2/SDL.h"
#include "external/include/SDL2/SDL_rect.h"
#include "external/include/SDL2/SDL_render.h"
#include "external/include/SDL2/SDL_error.h"
#include "external/include/SDL2/SDL_events.h"
#include "external/include/SDL2/SDL_surface.h"
#include "external/include/SDL2/SDL_timer.h"
#include "external/include/SDL2/SDL_scancode.h"
#include "external/include/SDL2/SDL_video.h"
#include "external/include/SDL2/SDL_keycode.h"
#include "external/include/SDL2/SDL_mouse.h"
#include "external/include/SDL2/SDL_pixels.h"
#include "external/include/SDL2/SDL_scancode.h"
#include "external/include/SDL2/SDL_syswm.h"

#include "external/include/SDL2_gfxPrimitives.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "./external/include/stb_image.h"
#include "./external/include/stb_image_write.h"

#ifdef _WIN32
    #include <winnt.h>
    #include <uxtheme.h>
    #include <windows.h>
    #include <dwmapi.h>
#endif

#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <immintrin.h> 

#include "./include/util.h"
#include "./include/font.h"

#define M_PI		                3.14159265358979323846
#define M_PI_2                      1.57079632679489661923

#define NUM_SIZES                   16

#define RGBA_TO_UINT32(r, g, b, a)  ((unsigned)(r) | ((unsigned)(g) << 8) | ((unsigned)(b) << 16) | ((unsigned)(a) << 24))
#define COLOR_BUF_AT(C,x,y)         (C)->pixels[(x)+(y)*C->width]

#define MAX3(a,b,c)                 ((a) > (b) ? ((a) > (c) ? (a) : (c)) : ((b) > (c) ? (b) : (c)))
#define MIN3(a,b,c)                 ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

typedef uint8_t  u8;
typedef int8_t   s8;

typedef uint32_t u32;
typedef int32_t  i32;

typedef float    f32;
typedef double   f64;


typedef struct color4_t
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
}color4_t;

typedef struct vec3f_t
{
    f32 x;
    f32 y;
    f32 z;
}vec3f_t;

typedef struct vec4f_t
{
    f32 x;
    f32 y;
    f32 z;
    f32 w;    
}vec4f_t;

#define ATTR_AT(a,i)   ((char const *)((a).ptr) + (a).stride * (i))
#define ATTR_NEW(p)    (attribute_t) {.ptr = (p), .stride = sizeof(typeof((p)[0]))}

typedef struct attribute_t
{
    void const *ptr;
    u32 stride;             // distance in bytes between two values
}attribute_t;   

typedef struct image_view_t
{
    color4_t    *pixels;
    u32         width;
    u32         height;
}image_view_t;

typedef struct mesh_t
{
    attribute_t     positions;
    attribute_t     colors;
    u32 const       *indices;
    u32             count;
}mesh_t;

typedef struct 
{
    float x;
    float y;
} Point;

typedef enum cull_mode_t
{
    CULL_MODE_NONE,
    CULL_MODE_CW,    // clockwise
    CULL_MODE_CCW    // counter-clockwise
}cull_mode_t;

typedef struct mat4x4_t
{
    float values[16];
}mat4x4_t;

typedef struct draw_command_t
{
    mesh_t          mesh;
    cull_mode_t     cull_mode;
    mat4x4_t        transform;
}draw_command_t;

typedef struct viewport_t 
{
    i32 xmin;
    i32 ymin;
    i32 xmax;
    i32 ymax;
}viewport_t;

struct context_t
{
    SDL_Window*         window;
    image_view_t        draw_buffer;
    u32                 screen_width;
    u32                 screen_height;
    u32                 mouseX;
    u32                 mouseY;
    /* FLAGS */
    bool                running;
    bool                resize;
    bool                rescale;
    bool                render;
    bool                dock;
    bool                capture;
    /* TIME */
    uint32_t            start_time;
    float               prev_time;
    float               dt;
    uint32_t            global_scale;
    /* STUFF */
    SDL_Cursor*         hand_cursor;
    SDL_Cursor*         arrow_cursor;
    SDL_Surface*        icon;
}gc;

float curr_time = 0.f;
SDL_Surface* draw_surface;

static vec3f_t cube_positions[] =
{
    // -X face
    {-1.f, -1.f, -1.f},
    {-1.f,  1.f, -1.f},
    {-1.f, -1.f,  1.f},
    {-1.f,  1.f,  1.f},

    // +X face
    { 1.f, -1.f, -1.f},
    { 1.f,  1.f, -1.f},
    { 1.f, -1.f,  1.f},
    { 1.f,  1.f,  1.f},

    // -Y face
    {-1.f, -1.f, -1.f},
    { 1.f, -1.f, -1.f},
    {-1.f, -1.f,  1.f},
    { 1.f, -1.f,  1.f},

    // +Y face
    {-1.f,  1.f, -1.f},
    { 1.f,  1.f, -1.f},
    {-1.f,  1.f,  1.f},
    { 1.f,  1.f,  1.f},

    // -Z face
    {-1.f, -1.f, -1.f},
    { 1.f, -1.f, -1.f},
    {-1.f,  1.f, -1.f},
    { 1.f,  1.f, -1.f},

    // +Z face
    {-1.f, -1.f,  1.f},
    { 1.f, -1.f,  1.f},
    {-1.f,  1.f,  1.f},
    { 1.f,  1.f,  1.f},
};

static color4_t cube_colors[] =
{
    // -X face (Purple to Pink gradient)
    {147.f, 51.f, 239.f, 255.f},     // Top left (Purple)
    {255.f, 191.f, 0.f, 255.f},      // Bottom left (Amber)
    {147.f, 51.f, 239.f, 255.f},     // Top right (Purple)
    {255.f, 0.f, 128.f, 255.f},      // Bottom right (Deep Pink)

    // +X face (Orange to Yellow gradient)
    {255.f, 128.f, 0.f, 255.f},      // Top left (Orange)
    {224.f, 64.f, 208.f, 255.f},     // Bottom left (Hot Pink)
    {255.f, 128.f, 0.f, 255.f},      // Top right (Orange)
    {255.f, 214.f, 0.f, 255.f},      // Bottom right (Yellow)

    // -Y face (Cyan to Blue gradient)
    {0.f, 108.f, 255.f, 255.f},      // Bottom left (Blue)
    {64.f, 224.f, 208.f, 255.f},     // Top right (Turquoise)
    {50.f, 205.f, 50.f, 255.f},      // Top left (Lime Green)
    {0.f, 128.f, 255.f, 255.f},      // Bottom right (Dodger Blue)

    // +Y face (Lime to Emerald gradient)
    {0.f, 191.f, 255.f, 255.f},      // Top left (Deep Sky Blue)
    {0.f, 128.f, 0.f, 255.f},        // Bottom left (Green)
    {199.f, 21.f, 133.f, 255.f},     // Bottom left (Medium Violet Red)
    {46.f, 139.f, 87.f, 255.f},      // Bottom right (Sea Green)

    // -Z face (Red to Purple gradient)
    {184.f, 115.f, 51.f, 255.f},     // Bottom left (Bronze)
    {218.f, 165.f, 32.f, 255.f},     // Top right (Golden Rod)
    {220.f, 20.f, 60.f, 255.f},      // Top right (Crimson)
    {148.f, 0.f, 211.f, 255.f},      // Bottom right (Dark Violet)

    // +Z face (Gold to Bronze gradient)
    {255.f, 0.f, 0.f, 255.f},        // Top left (Red)
    {255.f, 215.f, 0.f, 255.f},      // Top left (Gold)
    {205.f, 127.f, 50.f, 255.f},     // Bottom right (Peru)
    {124.f, 252.f, 0.f, 255.f},      // Top right (Lawn Green)
};

static u32 cube_indices[] =
{
    // -X face
     0,  2,  1,
     1,  2,  3,

    // +X face
     4,  5,  6,
     6,  5,  7,

    // -Y face
     8,  9, 10,
    10,  9, 11,

    // +Y face
    12, 14, 13,
    14, 15, 13,

    // -Z face
    16, 18, 17,
    17, 18, 19,

    // +Z face
    20, 21, 22,
    21, 23, 22,
};

mat4x4_t mat_identity(void)
{
    return (mat4x4_t){
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
}

mat4x4_t mat_scale(vec3f_t s)
{
    return (mat4x4_t) {
        s.x,  0.f,  0.f,  0.f,
        0.f,  s.y,  0.f,  0.f,
        0.f,  0.f,  s.z,  0.f,
        0.f,  0.f,  0.f,  1.f,
    };
}

mat4x4_t mat_scale_const(f32 s)
{
    vec3f_t vec = {
        .x = s,
        .y = s,
        .z = s,
    };
    return mat_scale(vec);
}

mat4x4_t mat_translate(vec3f_t s)
{
    return (mat4x4_t){
        1.f, 0.f, 0.f, s.x,
        0.f, 1.f, 0.f, s.y,
        0.f, 0.f, 1.f, s.z,
        0.f, 0.f, 0.f, 1.f,
    };
}

mat4x4_t mat_rotate_xy(float angle)
{
    float cos = cosf(angle);
    float sin = sinf(angle);

    return (mat4x4_t){
        cos, -sin, 0.f, 0.f,
        sin,  cos, 0.f, 0.f,
        0.f,  0.f, 1.f, 0.f,
        0.f,  0.f, 0.f, 1.f,
    };
}

mat4x4_t mat_rotate_yz(float angle)
{
    float cos = cosf(angle);
    float sin = sinf(angle);

    return (mat4x4_t){
        1.f, 0.f,  0.f, 0.f,
        0.f, cos, -sin, 0.f,
        0.f, sin,  cos, 0.f,
        0.f, 0.f,  0.f, 1.f,
    };
}

mat4x4_t mat_rotate_zx(float angle)
{
    float cos = cosf(angle);
    float sin = sinf(angle);

    return (mat4x4_t){
         cos, 0.f, sin, 0.f,
         0.f, 1.f, 0.f, 0.f,
        -sin, 0.f, cos, 0.f,
         0.f, 0.f, 0.f, 1.f,
    };
}

mat4x4_t mat_perspective(float n, float f, float fovY, float aspect_ratio)
{
    float top   = n * tanf(fovY / 2.f);
    float right = top * aspect_ratio;

    return (mat4x4_t) {
        n / right,      0.f,       0.f,                    0.f,
        0.f,            n / top,   0.f,                    0.f,
        0.f,            0.f,       -(f + n) / (f - n),     - 2.f * f * n / (f - n),
        0.f,            0.f,       -1.f,                   0.f,
    };
}

vec4f_t viewport_apply(viewport_t const *vp, vec4f_t v)
{
    v.x = (float)vp->xmin + (float)(vp->xmax - vp->xmin) * (0.5f + 0.5f * v.x);
    v.y = (float)vp->ymin + (float)(vp->ymax - vp->ymin) * (0.5f - 0.5f * v.y);
    return v;
}

vec4f_t vec4f_mat_mul_SIMD(mat4x4_t const *m, vec4f_t const *v) 
{
    __m128 vec = _mm_set_ps(v->w, v->z, v->y, v->x);

    __m128 col0 = _mm_loadu_ps(&m->values[0]);
    __m128 col1 = _mm_loadu_ps(&m->values[4]);
    __m128 col2 = _mm_loadu_ps(&m->values[8]);
    __m128 col3 = _mm_loadu_ps(&m->values[12]);

    __m128 result_x = _mm_mul_ps(col0, vec);
    __m128 result_y = _mm_mul_ps(col1, vec);
    __m128 result_z = _mm_mul_ps(col2, vec);
    __m128 result_w = _mm_mul_ps(col3, vec);

    float x = _mm_cvtss_f32(_mm_hadd_ps(_mm_hadd_ps(result_x, result_x), result_x));
    float y = _mm_cvtss_f32(_mm_hadd_ps(_mm_hadd_ps(result_y, result_y), result_y));
    float z = _mm_cvtss_f32(_mm_hadd_ps(_mm_hadd_ps(result_z, result_z), result_z));
    float w = _mm_cvtss_f32(_mm_hadd_ps(_mm_hadd_ps(result_w, result_w), result_w));

    return (vec4f_t){x, y, z, w};
}

vec4f_t vec4f_mat_mul(mat4x4_t const *m, vec4f_t const *v)
{
    return (vec4f_t){
        .x = m->values[ 0] * v->x + m->values[ 1] * v->y + m->values[ 2] * v->z + m->values[ 3] * v->w,
        .y = m->values[ 4] * v->x + m->values[ 5] * v->y + m->values[ 6] * v->z + m->values[ 7] * v->w,
        .z = m->values[ 8] * v->x + m->values[ 9] * v->y + m->values[10] * v->z + m->values[11] * v->w,
        .w = m->values[12] * v->x + m->values[13] * v->y + m->values[14] * v->z + m->values[15] * v->w
    };
}

inline vec4f_t perspective_divide(vec4f_t v)
{
    v.x /= v.w;
    v.y /= v.w;
    v.z /= v.w;
    return v;
}

mat4x4_t mat4x4_mult(mat4x4_t const *m, mat4x4_t const *n)
{
    float const m00 = m->values[0];
    float const m01 = m->values[1];
    float const m02 = m->values[2];
    float const m03 = m->values[3];
    float const m10 = m->values[4];
    float const m11 = m->values[5];
    float const m12 = m->values[6];
    float const m13 = m->values[7];
    float const m20 = m->values[8];
    float const m21 = m->values[9];
    float const m22 = m->values[10];
    float const m23 = m->values[11];
    float const m30 = m->values[12];
    float const m31 = m->values[13];
    float const m32 = m->values[14];
    float const m33 = m->values[15];

    float const n00 = n->values[0];
    float const n01 = n->values[1];
    float const n02 = n->values[2];
    float const n03 = n->values[3];
    float const n10 = n->values[4];
    float const n11 = n->values[5];
    float const n12 = n->values[6];
    float const n13 = n->values[7];
    float const n20 = n->values[8];
    float const n21 = n->values[9];
    float const n22 = n->values[10];
    float const n23 = n->values[11];
    float const n30 = n->values[12];
    float const n31 = n->values[13];
    float const n32 = n->values[14];
    float const n33 = n->values[15];

    mat4x4_t res;

    res.values[0]  = m00*n00+m10*n01+m20*n02+m30*n03;
    res.values[1]  = m01*n00+m11*n01+m21*n02+m31*n03;
    res.values[2]  = m02*n00+m12*n01+m22*n02+m32*n03;
    res.values[3]  = m03*n00+m13*n01+m23*n02+m33*n03;
    res.values[4]  = m00*n10+m10*n11+m20*n12+m30*n13;
    res.values[5]  = m01*n10+m11*n11+m21*n12+m31*n13;
    res.values[6]  = m02*n10+m12*n11+m22*n12+m32*n13;
    res.values[7]  = m03*n10+m13*n11+m23*n12+m33*n13;
    res.values[8]  = m00*n20+m10*n21+m20*n22+m30*n23;
    res.values[9]  = m01*n20+m11*n21+m21*n22+m31*n23;
    res.values[10] = m02*n20+m12*n21+m22*n22+m32*n23;
    res.values[11] = m03*n20+m13*n21+m23*n22+m33*n23;
    res.values[12] = m00*n30+m10*n31+m20*n32+m30*n33;
    res.values[13] = m01*n30+m11*n31+m21*n32+m31*n33;
    res.values[14] = m02*n30+m12*n31+m22*n32+m32*n33;
    res.values[15] = m03*n30+m13*n31+m23*n32+m33*n33;
    
    return res;
}

mat4x4_t mat4x4_mult_simd(mat4x4_t const *m, mat4x4_t const *n) 
{
    mat4x4_t res;
    
    __m128 row0 = _mm_load_ps(&m->values[0]);
    __m128 row1 = _mm_load_ps(&m->values[4]);
    __m128 row2 = _mm_load_ps(&m->values[8]);
    __m128 row3 = _mm_load_ps(&m->values[12]);
    
    for (int i = 0; i < 4; i++) 
    {
        __m128 brod0 = _mm_set1_ps(n->values[i]);
        __m128 brod1 = _mm_set1_ps(n->values[i + 4]);
        __m128 brod2 = _mm_set1_ps(n->values[i + 8]);
        __m128 brod3 = _mm_set1_ps(n->values[i + 12]);
        
        __m128 result = _mm_mul_ps(row0, brod0);
        result = _mm_add_ps(result, _mm_mul_ps(row1, brod1));
        result = _mm_add_ps(result, _mm_mul_ps(row2, brod2));
        result = _mm_add_ps(result, _mm_mul_ps(row3, brod3));
        
        _mm_store_ps(&res.values[i * 4], result);
    }
    
    return res;
}


SDL_Surface *surface_from_image(const char *path)
{
    int req_format = STBI_rgb_alpha;
    int width, height, n;
    unsigned char *pixels = stbi_load(path, &width, &height, &n, req_format);
    if(pixels == NULL){
        fprintf(stdout,"Couldnt load file : %s because :  %s\n",path, stbi_failure_reason());
        exit(1);
    }
    Uint32 rmask, gmask, bmask, amask;
    #if SDL_BYTEORDER == SDL_BIG_ENDIAN
        int shift = (req_format == STBI_rgb) ? 8 : 0;
        rmask = 0xff000000 >> shift;
        gmask = 0x00ff0000 >> shift;
        bmask = 0x0000ff00 >> shift;
        amask = 0x000000ff >> shift;
    #else // little endian, like x86
        rmask = 0x000000ff;
        gmask = 0x0000ff00;
        bmask = 0x00ff0000;
        amask = 0xff000000;
    #endif

    int depth, pitch;

    depth = 32;
    pitch = 4*width;

    SDL_Surface* surf = (SDL_Surface *)CHECK_PTR(SDL_CreateRGBSurfaceFrom((void*)pixels, width, height, depth, pitch, rmask, gmask, bmask, amask));

    return surf;
}

void set_dark_mode(SDL_Window *window)
{
    #ifdef _WIN32
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window, &wmInfo);
        HWND hwnd = wmInfo.info.win.window;

        BOOL value = TRUE;
        DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
        SetWindowTheme(hwnd, L"DarkMode_Explorer", NULL);
        SetWindowPos(hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    #endif
}

SDL_Color get_rand_color(void) 
{
    srand((unsigned int)time(NULL));
    
    Uint8 r = rand() % 256;
    Uint8 g = rand() % 256;
    Uint8 b = rand() % 256;

    SDL_Color color = { r, g, b, 255 }; 
    return color;
}


float sign(Point p1, Point p2, Point p3) 
{
    return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

/* 
    only points to the left of all three edges
    are precisely the points inside the triangle
 */
bool isPointInTriangle(Point pt, Point v1, Point v2, Point v3) 
{
    bool b1, b2, b3;
    
    b1 = sign(pt, v1, v2) < 0.0f;
    b2 = sign(pt, v2, v3) < 0.0f;
    b3 = sign(pt, v3, v1) < 0.0f;
    
    return ((b1 == b2) && (b2 == b3));
}


/*
    convert normalized color to 0->255 range
*/
inline color4_t to_color4(vec4f_t const c)
{
    color4_t res;
    res.r = MAX(0.0f,MIN(255.0f, c.x * 255.0f));
    res.g = MAX(0.0f,MIN(255.0f, c.y * 255.0f));
    res.b = MAX(0.0f,MIN(255.0f, c.z * 255.0f));
    res.a = MAX(0.0f,MIN(255.0f, c.w * 255.0f));

    return res;
}

inline vec4f_t vec4f_as_vector(vec3f_t const  *v)
{
    return (vec4f_t){v->x, v->y, v->z, 0.0f};
}

inline vec4f_t vecf4_as_point(vec3f_t const *v)
{
    return (vec4f_t){v->x, v->y, v->z, 1.0f};
}

inline vec4f_t vec4f_sub (vec4f_t const *v0, vec4f_t const  *v1)
{
    return (vec4f_t){v0->x - v1->x, v0->y - v1->y, v0->z - v1->z, v0->w - v1->w};
}

inline void vecf4_swap (vec4f_t *v0, vec4f_t *v1)
{
    vec4f_t const tmp = *v0;
    *v0 = *v1;
    *v1 = tmp;
}

inline void color4_swap (color4_t *c0, color4_t *c1)
{

    color4_t const tmp = *c0;

    *c0 = *c1;
    *c1 = tmp;
}

inline float vec4f_det2D(vec4f_t const *v0, vec4f_t const* v1)
{
    return v0->x * v1->y - v0->y * v1->x;
}

/* ----------------  Events -------------------- */
void poll_events()
{
    //ZoneScoped;

    SDL_Event event = {0};
    const Uint8 *keyboard_state_array = SDL_GetKeyboardState(NULL);

    while (SDL_PollEvent(&event)) 
    {
        switch (event.type) 
        {
            case SDL_QUIT:{
                gc.running = false;
            }break;

            case SDL_WINDOWEVENT:{
                switch (event.window.event)
                {
                    case SDL_WINDOWEVENT_RESIZED:
                        if(draw_surface){
                            SDL_FreeSurface(draw_surface);
                        }
                        draw_surface     = NULL;
                        gc.screen_width  = event.window.data1;
                        gc.screen_height = event.window.data2;
                        gc.resize        = true;
                        break;
                }
            }break;

            case SDL_MOUSEBUTTONDOWN:

            break;

            case SDL_MOUSEMOTION:
                gc.mouseX = event.motion.x;
                gc.mouseY = event.motion.y;
                break;

           case SDL_KEYUP:{
                switch(event.key.keysym.sym){
                    case SDLK_RSHIFT:
                    case SDLK_LSHIFT:
                    default:
                        break;
                }
            }break;
    
            case SDL_KEYDOWN:{
                if ((event.key.keysym.sym >= SDLK_0 && event.key.keysym.sym <= SDLK_9)) {

                }
                switch(event.key.keysym.sym)
                {
                    case SDLK_ESCAPE:
                        break;
                    case SDLK_BACKSPACE:
                        break;
                    case SDLK_RETURN:
                        break;
                    case SDLK_LEFT:
                        break;
                    case SDLK_RIGHT:
                        break;
                    case SDLK_UP:
                        break;
                    case SDLK_DOWN:
                        break;
                    case SDLK_PAGEUP:
                        break;
                    case SDLK_PAGEDOWN:
                        break;
                    case SDLK_HOME:
                    break;
                    case SDLK_F11:
                        gc.dock ^=1;
                        gc.capture = true;
                    break;
                    case SDLK_END:
                    break;
                    case SDLK_TAB:
                    break;
                    case SDLK_LSHIFT:
                    case SDLK_RSHIFT:
                    break;
                    case SDLK_PRINTSCREEN:
                        break;
                    default:
                        break;
                }
                if ((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_Q]))
                {
                    gc.running = false;
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        ((keyboard_state_array[SDL_SCANCODE_KP_PLUS]) 
                        || (keyboard_state_array[SDL_SCANCODE_EQUALS])))
                {
                    if(gc.global_scale < NUM_SIZES){
                        gc.global_scale++;
                        gc.rescale = true;
                    }
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        ((keyboard_state_array[SDL_SCANCODE_KP_MINUS]) 
                        || (keyboard_state_array[SDL_SCANCODE_KP_MINUS])))
                {
                    if(gc.global_scale > 0){
                        gc.global_scale--;
                        gc.rescale = true;
                    }
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_HOME]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_END]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_RIGHT]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_PAGEDOWN]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_PAGEUP]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_S]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_C]))
                {
                }
                else if((keyboard_state_array[SDL_SCANCODE_LCTRL]) &&
                        (keyboard_state_array[SDL_SCANCODE_V]))
                {
                    // char * SDL_GetClipboardText(void);
                }
            }break;

            default:
                return;
        }
        return;
    }
}

int EventWatch(void *userdata, SDL_Event *event) 
{
    (void) userdata;
    
    if (event->type == SDL_WINDOWEVENT && event->window.event == SDL_WINDOWEVENT_RESIZED) {
        gc.resize = true;
        SDL_FlushEvent(SDL_WINDOWEVENT_RESIZED);
        return 0;
    }
    return 1;
}

void clear_screen(image_view_t const *color_buf, color4_t const color)
{
    for (int y = 0; y < color_buf->height; ++y){
        for(int x = 0; x < color_buf->width; ++x){        
            color_buf->pixels[x+y*color_buf->width] = color;
        }
    }
}


void draw_triangle(image_view_t const *color_buf, Point v1, Point v2, Point v3)
{
    /* Bounding Box */
    int minX = MIN3(v1.x, v2.x, v3.x);
    int minY = MIN3(v1.y, v2.y, v3.y);
    int maxX = MAX3(v1.x, v2.x, v3.x);
    int maxY = MAX3(v1.y, v2.y, v3.y);

    minX = MAX(minX, 0);
    minY = MAX(minY, 0);
    maxX = MIN(maxX, gc.screen_width - 1);
    maxY = MIN(maxY, gc.screen_height - 1);

    Point p;
    for (p.y = minY; p.y <= maxY; p.y++){
        for (p.x = minX; p.x <= maxX; p.x++){
            if(isPointInTriangle(p, v1, v2, v3))
            {
                color_buf->pixels[(int)p.x+(int)p.y*color_buf->width] = to_color4((vec4f_t){150,150,150,255});
            }
        }
    }
}

void swap(int* a, int* b) 
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

void draw_line(image_view_t const *color_buf, int x0, int y0, int x1, int y1, vec4f_t const color)
{
    bool steep = false;
    
    // If the line is steep, we transpose the image
    if (abs(x0 - x1) < abs(y0 - y1)) {
        swap(&x0, &y0);
        swap(&x1, &y1);
        steep = true;
    }
    
    // Make it left-to-right
    if (x0 > x1) {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }
    
    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    
    for (int x = x0; x <= x1; x++) {
        if (steep) {
            color_buf->pixels[y+x*color_buf->width] = to_color4(color);
        } else {
            color_buf->pixels[x+y*color_buf->width] = to_color4(color);
        }
        
        error2 += derror2;
        if (error2 > dx) {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

void draw_mesh(image_view_t const *color_buf, draw_command_t const *command, viewport_t const *vp)
{
    for(size_t vidx = 0;
        vidx + 2 < command->mesh.count;
        vidx+=3)
    {
        u32 i0 = vidx+0;
        u32 i1 = vidx+1;
        u32 i2 = vidx+2;

        if(command->mesh.indices)
        {
            i0 = command->mesh.indices[i0];
            i1 = command->mesh.indices[i1];
            i2 = command->mesh.indices[i2];
        }

        vec4f_t p0 = vecf4_as_point((vec3f_t *)ATTR_AT(command->mesh.positions, i0));
        vec4f_t p1 = vecf4_as_point((vec3f_t *)ATTR_AT(command->mesh.positions, i1));
        vec4f_t p2 = vecf4_as_point((vec3f_t *)ATTR_AT(command->mesh.positions, i2));

        vec4f_t v0 = vec4f_mat_mul(&command->transform, &p0);
        vec4f_t v1 = vec4f_mat_mul(&command->transform, &p1);
        vec4f_t v2 = vec4f_mat_mul(&command->transform, &p2);

        v0 = perspective_divide(v0);
        v1 = perspective_divide(v1);
        v2 = perspective_divide(v2);

        v0 = viewport_apply(vp, v0);
        v1 = viewport_apply(vp, v1);
        v2 = viewport_apply(vp, v2);

        color4_t c0 = *(color4_t *)ATTR_AT(command->mesh.colors, i0);
        color4_t c1 = *(color4_t *)ATTR_AT(command->mesh.colors, i1);
        color4_t c2 = *(color4_t *)ATTR_AT(command->mesh.colors, i2);

        vec4f_t v10 = vec4f_sub(&v1, &v0);
        vec4f_t v20 = vec4f_sub(&v2, &v0);  

        float det012 = vec4f_det2D(&v10, &v20);

        // is it counter-clockwise
        bool const ccw = det012 < 0.f;

        switch(command->cull_mode)
        {
            case CULL_MODE_NONE:
                break;
            case CULL_MODE_CW:
                if(!ccw)
                    continue;
                break;
            case CULL_MODE_CCW:
                if(ccw)
                    continue;
                break;
        }

        if (ccw){
            vecf4_swap(&v1, &v2);
            color4_swap(&c1, &c2);
            det012 = -det012;
        }

        // Bounding Box
        i32 xmin = MAX(vp->xmin, 0);
        i32 xmax = MIN(vp->xmax, (i32)color_buf->width)-1;
        i32 ymin = MAX(vp->ymin, 0);
        i32 ymax = MIN(vp->ymax, (i32)color_buf->height)-1;

        xmin = MAX(xmin, MIN3(floor(v0.x), floor(v1.x), floor(v2.x)));
        xmax = MIN(xmax, MAX3(ceil(v0.x), ceil(v1.x), ceil(v2.x)));
        ymin = MAX(ymin, MIN3(floor(v0.y), floor(v1.y), floor(v2.y)));
        ymax = MIN(ymax, MAX3(ceil(v0.y), ceil(v1.y), ceil(v2.y)));

        for (i32 y = ymin; y < ymax; ++y)
        {
            for (i32 x = xmin; x < xmax; ++x)
            {
                // point is considered in the middle of the pixel
                vec4f_t p = {x + 0.5f, y + 0.5f, 0.f, 0.f};     

                vec4f_t v10 = vec4f_sub(&v1, &v0);
                vec4f_t vp0 = vec4f_sub(&p, &v0);

                vec4f_t v21 = vec4f_sub(&v2, &v1);
                vec4f_t vp1 = vec4f_sub(&p, &v1);

                vec4f_t v02 = vec4f_sub(&v0, &v2);
                vec4f_t vp2 = vec4f_sub(&p, &v2);

                float det01p = vec4f_det2D(&v10, &vp0);
                float det12p = vec4f_det2D(&v21, &vp1);
                float det20p = vec4f_det2D(&v02, &vp2);

                if (det01p >= 0.0f && det12p >= 0.0f && det20p >= 0.0f)
                {
                    float l0 = det12p / det012;
                    float l1 = det20p / det012;
                    float l2 = det01p / det012;

                    color4_t final_col = {
                        .r = c0.r * l0 + c1.r * l1 + c2.r * l2,
                        .g = c0.g * l0 + c1.g * l1 + c2.g * l2,
                        .b = c0.b * l0 + c1.b * l1 + c2.b * l2,
                        .a = 255
                    };
                      
                    COLOR_BUF_AT(color_buf, x, y) = final_col;
                }
            }
        }
    }
}

#define TGA_HEADER(buf,w,h,b) \
    header[2]  = 2;\
    header[12] = (w) & 0xFF;\
    header[13] = ((w) >> 8) & 0xFF;\
    header[14] = (h) & 0xFF;\
    header[15] = ((h) >> 8) & 0xFF;\
    header[16] = (b)

void export_image(image_view_t const *color_buf, const char *filename) 
{
    FILE *file = fopen(filename, "wb");

    if (!file) {
        perror("Failed to open file");
        return;
    }

    uint8_t header[18] = {0};
    TGA_HEADER(header, color_buf->width, color_buf->height, 32);

    fwrite(header, sizeof(uint8_t), 18, file);

    for (int y = 0; y < color_buf->height; ++y) {
        for (int x = 0; x < color_buf->width; ++x) {
            color4_t pixel = COLOR_BUF_AT(color_buf, x, y);
            uint8_t bgra[4] = { pixel.b, pixel.g, pixel.r, pixel.a };
            fwrite(bgra, sizeof(uint8_t), 4, file);
        }
    }
    
    fclose(file);
}

void render_all()
{
    curr_time += gc.dt;

    if(!draw_surface)
    {
        draw_surface = SDL_CreateRGBSurfaceWithFormat(0, gc.screen_width, gc.screen_height, 32, SDL_PIXELFORMAT_RGBA32);
        SDL_SetSurfaceBlendMode(draw_surface, SDL_BLENDMODE_NONE);
        gc.draw_buffer.pixels = (color4_t *)draw_surface->pixels;
        gc.draw_buffer.height = gc.screen_height;
        gc.draw_buffer.width  = gc.screen_width;
    }
    
    clear_screen(&gc.draw_buffer, (color4_t){40.f, 42.f, 54.f, 255.f});

    // draw_triangle(&gc.draw_buffer,(Point){100,100},(Point){200,100}, (Point){100,200});

/*  vec3f_t positions[] =
    {
        {-0.5f, -0.5f, 0.f},
        {-0.5f,  0.5f, 0.f},
        { 0.5f, -0.5f, 0.f},
        { 0.5f,  0.5f, 0.f},
    };

    color4_t colors[] =
    {
        {255, 0, 0, 255},
        {0, 255, 0, 255},
        {0, 0, 255, 255},
        {255, 255, 255, 255},
    };

    u32 indices[] = {
        0, 1, 2,
        2, 1, 3,
    };
*/
    viewport_t vp = {
        .xmin = 0,
        .ymin = 0,
        .xmax = (i32)gc.draw_buffer.width,
        .ymax = (i32)gc.draw_buffer.height
    };

/*    
    draw_command_t cmd = {
        .mesh = {
            .positions    = ATTR_NEW(positions),
            .colors       = ATTR_NEW(colors),
            .indices      = indices,
            .count        = 6,
        },
        .transform = mat_rotate_zx(curr_time)
    };
*/
    float width_scale  = MIN(1.0f, gc.screen_height * 1.0f / gc.screen_width);
    float height_scale = MIN(1.0f, gc.screen_width * 1.0f / gc.screen_height);

    mat4x4_t aspect      = mat_scale((vec3f_t){width_scale, height_scale, 1.f});
    mat4x4_t scale       = mat_scale_const(0.5f);
    mat4x4_t rotatezx    = mat_rotate_zx(curr_time);
    mat4x4_t rotatexy    = mat_rotate_xy(curr_time * 1.61f);
    mat4x4_t perspective = mat_perspective(0.01f, 10.f, M_PI / 3.f, 1.0f);
    mat4x4_t translate   = mat_translate((vec3f_t){0.f, 0.f, -5.f});

    mat4x4_t transform = mat4x4_mult(&scale, &rotatezx);        // First apply scale
    transform = mat4x4_mult(&transform, &rotatexy);             // Then rotations
    transform = mat4x4_mult(&transform, &translate);            // Then translation
    transform = mat4x4_mult(&transform, &perspective);          // Then perspective
    transform = mat4x4_mult(&transform, &aspect);            // Finally aspect ratio correction

    draw_command_t cmd = {
        .mesh = {
            .positions = ATTR_NEW(cube_positions),
            .colors = ATTR_NEW(cube_colors),
            .indices = cube_indices,
            .count = 36,
        },
        .transform = transform,
        .cull_mode = CULL_MODE_CW
    };

    draw_mesh(&gc.draw_buffer, &cmd, &vp);
    // draw_line(&gc.draw_buffer,0,0,gc.screen_width,gc.screen_height,(vec4f_t){0.0f, 0.0f, 0.5f, 1.0f});

    SDL_Rect rect = {.x = 0, .y = 0, .w = gc.screen_width, .h = gc.screen_height};

    SDL_BlitSurface(draw_surface, &rect, SDL_GetWindowSurface(gc.window), &rect);
    SDL_UpdateWindowSurface(gc.window);

    if(gc.capture){
        export_image(&gc.draw_buffer, "test.tga");
        gc.capture = false;
    }
}


void init_all(void)
{
    gc.screen_width  = 800;
    gc.screen_height = 600;

    LOG_ERROR(SDL_Init(SDL_INIT_VIDEO));
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); 

    gc.window =  (SDL_Window *)CHECK_PTR(SDL_CreateWindow("3D Renderer",
                                                          SDL_WINDOWPOS_CENTERED,
                                                          SDL_WINDOWPOS_CENTERED,
                                                          gc.screen_width, gc.screen_height,
                                                          SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN));

    SDL_AddEventWatch(EventWatch, NULL);

    gc.hand_cursor  = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND); 
    gc.arrow_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    gc.icon         = surface_from_image("..\\Images\\icon.png");

    SDL_SetWindowIcon(gc.window, gc.icon);

    gc.running     = true;
    gc.resize      = true;
    gc.rescale     = true;
    gc.render      = true;
    gc.dock        = false;

    gc.global_scale = 1;

    set_dark_mode(gc.window);
}

float get_time_difference(void *last_time) 
{
    float dt = 0.0f;

#ifdef WIN32
    LARGE_INTEGER now, frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&now);
    LARGE_INTEGER *last_time_win = (LARGE_INTEGER *)last_time;
    dt = (float)(now.QuadPart - last_time_win->QuadPart) / frequency.QuadPart;
    *last_time_win = now;
#else
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    struct timespec *last_time_posix = (struct timespec *)last_time;
    dt = (now.tv_sec - last_time_posix->tv_sec) +
         (now.tv_nsec - last_time_posix->tv_nsec) / 1e9f;
    *last_time_posix = now;
#endif

    return dt;
}


int main(int argc, char* argv[])
{   
    (void) argv;
    (void) argc;

    init_all();

    gc.prev_time = SDL_GetTicks();

#ifdef WIN32
    LARGE_INTEGER last_frame_start;
    QueryPerformanceCounter(&last_frame_start);
#else
    struct timespec last_frame_start;
    clock_gettime(CLOCK_MONOTONIC, &last_frame_start);
#endif

    while(gc.running)
    {
        poll_events();

        // gc.start_time = SDL_GetTicks();

        // gc.dt = (gc.start_time - gc.prev_time);
        // gc.prev_time = gc.start_time;
        // printf("%f\n", gc.dt);

        gc.dt = get_time_difference(&last_frame_start);
        // printf("%f\n", gc.dt*1000);

        render_all();

        // float frame_time = (SDL_GetTicks() - gc.start_time);
        // float elapsedTime = (frame_time > 0) ? frame_time : 1;
        
        // if(elapsedTime < FPS(60)){
             // SDL_Delay(FPS(60)-elapsedTime);
         // }
    }
    SDL_Quit();
    return 0;
}
 