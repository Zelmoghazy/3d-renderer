#include "../include/util.h"

void log_error(int error_code, const char* file, int line)
{
    if (error_code != 0){
        fprintf(stderr,"In file: %s, Line : %d \nSDL ERROR : %s",  file, line, SDL_GetError());
        exit(1);
    }
}

void* check_ptr(void *ptr, const char* file, int line)
{
    if(ptr == NULL){
        fprintf(stderr, "In file: %s, Line : %d \nSDL NULL ptr : %s", file, line, SDL_GetError());
        exit(1);
    }
    return ptr;
}

void buf_append(abuf *ab, const char *s, int len) 
{
    char *new_buf = (char *)realloc(ab->b, ab->len + len);
    if (new_buf == NULL) return;
    memcpy(&new_buf[ab->len], s, len);
    ab->b = new_buf;
    ab->len += len;
}

void buf_free(abuf *ab) 
{
    free(ab->b);
}

vec2f_t vec2f(float x, float y)
{
    vec2f_t vec2 = {
        .x = x,
        .y = y,
    };
    return  vec2;
}

vec2f_t vec2f_add(vec2f_t a, vec2f_t b)
{
    vec2f_t vec2;
    vec2.x = a.x + b.x;
    vec2.y = a.y + b.y;
    return vec2;
}

vec2f_t vec2f_sub(vec2f_t a, vec2f_t b)
{
    vec2f_t vec2;
    vec2.x = a.x - b.x;
    vec2.y = a.y - b.y;
    return vec2;
}

vec2f_t vec2f_mul(vec2f_t a, vec2f_t b)
{
    vec2f_t vec2;
    vec2.x = a.x * b.x;
    vec2.y = a.y * b.y;
    return vec2;
}

vec2f_t vec2f_div(vec2f_t a, vec2f_t b)
{
    vec2f_t vec2;
    vec2.x = a.x / b.x;
    vec2.y = a.y / b.y;
    return vec2;
}

float values[] = { 
    0.0000f,0.0175f,0.0349f,0.0523f,0.0698f,0.0872f,0.1045f,0.1219f,
    0.1392f,0.1564f,0.1736f,0.1908f,0.2079f,0.2250f,0.2419f,0.2588f,
    0.2756f,0.2924f,0.3090f,0.3256f,0.3420f,0.3584f,0.3746f,0.3907f,
    0.4067f,0.4226f,0.4384f,0.4540f,0.4695f,0.4848f,0.5000f,0.5150f,
    0.5299f,0.5446f,0.5592f,0.5736f,0.5878f,0.6018f,0.6157f,0.6293f,
    0.6428f,0.6561f,0.6691f,0.6820f,0.6947f,0.7071f,0.7071f,0.7193f,
    0.7314f,0.7431f,0.7547f,0.7660f,0.7771f,0.7880f,0.7986f,0.8090f,
    0.8192f,0.8290f,0.8387f,0.8480f,0.8572f,0.8660f,0.8746f,0.8829f,
    0.8910f,0.8988f,0.9063f,0.9135f,0.9205f,0.9272f,0.9336f,0.9397f,
    0.9455f,0.9511f,0.9563f,0.9613f,0.9659f,0.9703f,0.9744f,0.9781f,
    0.9816f,0.9848f,0.9877f,0.9903f,0.9925f,0.9945f,0.9962f,0.9976f,
    0.9986f,0.9994f,0.9998f,1.0000f
};

float sine(int x)
{
    x = x % 360;
    if (x < 0) {
        x += 360;
    }
    if (x == 0){
        return 0;
    }else if (x == 90){
        return 1;
    }else if (x == 180){
        return 0;
    }else if (x == 270){
        return -1;
    }

    if(x > 270){
        return -values[360-x];
    }else if(x>180){
        return -values[x-180];
    }else if(x>90){
        return values[180-x];
    }else{
        return values[x];
    }
}

float cosine(int x){
    return sine(90-x);
}

float length_vec3(float x, float y, float z) {
    return sqrtf(x*x + y*y + z*z);
}