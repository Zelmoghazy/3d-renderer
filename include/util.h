#ifndef UTIL_H_
#define UTIL_H_

#include "../external/include/SDL2/SDL_error.h"


#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#define TAB_SIZE 4

#define DEBUG  1

#define DEBUG_PRT(fmt, ...)                                                                                  \
    do{                                                                                                      \
        if(DEBUG)                                                                                            \
            fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__);     \
    }while(0)

#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define HEX_TO_RGBA(s,hex)      \
    s.r = ((hex >> 16) & 0xFF); \
    s.g = ((hex >> 8) & 0xFF);  \
    s.b = (hex & 0xFF);         \
    s.a = 255                   

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))


#define COLOR_RED           (SDL_Color){255, 0, 0, 255}
#define COLOR_GREEN         (SDL_Color){0, 255, 0, 255}
#define COLOR_BLUE          (SDL_Color){0, 0, 255, 255}
#define COLOR_WHITE         (SDL_Color){255, 255, 255, 255}
#define COLOR_BLACK         (SDL_Color){0, 0, 0, 255}
#define COLOR_YELLOW        (SDL_Color){255, 255, 0, 255}
#define COLOR_CYAN          (SDL_Color){0, 255, 255, 255}
#define COLOR_MAGENTA       (SDL_Color){255, 0, 255, 255}
#define COLOR_ORANGE        (SDL_Color){255, 165, 0, 255}
#define COLOR_PURPLE        (SDL_Color){128, 0, 128, 255}
#define COLOR_PINK          (SDL_Color){255, 192, 203, 255}
#define COLOR_GRAY          (SDL_Color){128, 128, 128, 255}
#define COLOR_LIGHT_GRAY    (SDL_Color){211, 211, 211, 255}
#define COLOR_DARK_GRAY     (SDL_Color){169, 169, 169, 255}
#define COLOR_BROWN         (SDL_Color){139, 69, 19, 255}
#define COLOR_NAVY          (SDL_Color){0, 0, 128, 255}
#define COLOR_LIME          (SDL_Color){0, 255, 0, 255}
#define COLOR_TEAL          (SDL_Color){0, 128, 128, 255}
#define COLOR_MAROON        (SDL_Color){128, 0, 0, 255}
#define COLOR_OLIVE         (SDL_Color){128, 128, 0, 255}


#define FPS(n)  (1000/n)

#define ABUF_INIT() {NULL, 0}

typedef struct vec2f_t{
    float x;
    float y;
}vec2f_t;

typedef struct vec2_t{
    int x;
    int y;
}vec2_t;


typedef struct abuf {
  char *b;
  int len;
}abuf;

void  log_error(int error_code, const char* file, int line);
void *check_ptr (void *ptr, const char* file, int line);

void buf_append(abuf *ab, const char *s, int len);
void buf_free(abuf *ab);

// ssize_t getdelim(char **buf, size_t *bufsiz, int delimiter, FILE *fp);
// ssize_t getline(char **buf, size_t *bufsiz, FILE *fp);

vec2f_t vec2f(float x, float y);
vec2f_t vec2f_add(vec2f_t a, vec2f_t b);
vec2f_t vec2f_sub(vec2f_t a, vec2f_t b);
vec2f_t vec2f_mul(vec2f_t a, vec2f_t b);
vec2f_t vec2f_div(vec2f_t a, vec2f_t b);

#define LOG_ERROR(error_code)   log_error(error_code, __FILE__, __LINE__)
#define CHECK_PTR(ptr)          check_ptr(ptr, __FILE__, __LINE__)


#endif /* UTIL_H_ */