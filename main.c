#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef long double real;

const int max_iteration = 500;
const real scale = 4.0;

int width = 1280 * scale;
int height = 720 * scale;

SDL_Texture *texture;
SDL_Surface *surface;
SDL_Renderer *renderer;

typedef union {
    uint8_t rgb[4];
    uint32_t color;
} Color;

int Clamp(int i) {
    if (i < 0) return 0;
    if (i > 255) return 255;
    return i;
}

/* Stole this from the internet somewhere */
void HsvToRgb(real H, real S, real V, Color *c) {    
    real R, G, B;

    /* while (H < 0) { H += 360; }; */
    /* while (H >= 360) { H -= 360; }; */

    if (V <= 0) {
        R = G = B = 0;
    } else if (S <= 0) {
        R = G = B = V;
    } else {
        real hf = H / 60.0;
        int i = (int)hf;
        real f = hf - i;
        real pv = V * (1 - S);
        real qv = V * (1 - S * f);
        real tv = V * (1 - S * (1 - f));
        switch (i) {
        case 0:
            R = V;
            G = tv;
            B = pv;
            break;
        case 1:
            R = qv;
            G = V;
            B = pv;
            break;
        case 2:
            R = pv;
            G = V;
            B = tv;
            break;
        case 3:
            R = pv;
            G = qv;
            B = V;
            break;
        case 4:
            R = tv;
            G = pv;
            B = V;
            break;
        case 5:
            R = V;
            G = pv;
            B = qv;
            break;
        case 6:
            R = V;
            G = tv;
            B = pv;
            break;
        case -1:
            R = V;
            G = pv;
            B = qv;
            break;
        default:
            R = G = B = V;
            break;
        }
    }

    c->rgb[0] = Clamp((int)(R * 255.0));
    c->rgb[1] = Clamp((int)(G * 255.0));
    c->rgb[2] = Clamp((int)(B * 255.0));
    c->rgb[3] = 255;
}

real map(real v, real min, real max, real start, real end) {
    return start + (end - start) * (v / (max - min));
}

void set_pixel(SDL_Surface *surface, int x, int y, uint32_t pixel) {
    uint32_t *const target_pixel = (uint32_t *) ((uint8_t *) surface->pixels
                                              + y * surface->pitch
                                              + x * surface->format->BytesPerPixel);
    *target_pixel = pixel;
}

void mandelbrot(const SDL_Rect *view) {
    int   x, y;
    real  xstart, xend, ystart, yend;

    if (texture) {
        SDL_DestroyTexture(texture);
    }
    
    xstart = -2.5 + (((view->x) / (real)width) * 3.5);
    xend   = -2.5 + (((view->x + view->w) / (real)width) * 3.5);
    ystart = -1.0 + ((view->y / (real)height) * 2.0);
    yend   = -1.0 + (((view->y + view->h) / (real)height) * 2.0);
    
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            Color  color;
            real   h, s, v;
            real   x0, y0, x2, y2, xx, yy;
            int    iteration;
            
            real   c;

            x0 = map(x, 0, width, xstart, xend);
            y0 = map(y, 0, height, ystart, yend);

            xx = 0.0;
            yy = 0.0;

            x2 = 0.0;
            y2 = 0.0;

            iteration = 0;
            
            while (x2 + y2 <= 4 && iteration < max_iteration) {
                yy = 2 * xx * yy + y0;
                xx = x2 - y2 + x0;
                x2 = xx * xx;
                y2 = yy * yy;
                iteration++;
            }

            c = (real)iteration / max_iteration;
            
            h = 360.0 * c;
            s = 1.0;

            if (iteration < max_iteration) {
                v = 1.0;
            } else {
                v = 0.0;
            }

            HsvToRgb(h, s, v, &color);

            set_pixel(surface, x, y, color.color);
        }
    }

    texture = SDL_CreateTextureFromSurface(renderer, surface);
}

int main(int argc, char **argv) {
    bool         running, test, escape;

    SDL_Window  *window;
    SDL_Rect     rectangle, view;
    SDL_Event    event;

    uint32_t     rmask, gmask, bmask, amask, mouse;
    int          mx, my;

    running = true;
    test = true;
    escape = false;
    
    SDL_Init(SDL_INIT_EVERYTHING);
    IMG_Init(IMG_INIT_PNG);
    
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    
    window = SDL_CreateWindow("Mandelbrot",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              width / scale,
                              height / scale,
                              0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    rectangle.x = rectangle.y = rectangle.w = rectangle.h = -1;
    view.x = 0;
    view.y = 0;
    view.w = width;
    view.h = height;
    
    surface = SDL_CreateRGBSurface(0, width, height,
                                   32, rmask, gmask, bmask, amask);

    mandelbrot(&view);

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    rectangle.x = rectangle.y = -1;
                    test = true;
                    escape = true;
                }
                if (event.key.keysym.sym == SDLK_r) {
                    view.x = 0;
                    view.y = 0;
                    view.w = width;
                    view.h = height;

                    mandelbrot(&view);
                }
                if (event.key.keysym.sym == SDLK_p) {
                    char str[64];
                    sprintf(str, "out/out_%d.png", time(NULL));
                    
                    IMG_SavePNG(surface, str);
                }
            }
        }

        mouse = SDL_GetMouseState(&mx, &my);

        if (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) {
            if (!escape) {
                if (rectangle.x == -1) {
                    rectangle.x = mx * scale;
                    rectangle.y = my * scale;
                } else {
                    rectangle.w = (mx * scale) - rectangle.x;
                    rectangle.h = rectangle.w / (real)width * (real)height;
                }
                test = false;
            }
        } else {
            if (!escape && !test) {
                /* Convert screen coordinates to world coordinates */
                view.x += view.w * rectangle.x / width;
                view.y += view.h * rectangle.y / height;
                view.w  = view.w * rectangle.w / width;
                view.h  = view.h * rectangle.h / height;
                
                test = true;

                rectangle.x = rectangle.y = rectangle.w = rectangle.h = -1;

                texture = NULL;

                mandelbrot(&view);
            }
            escape = false;
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        if (texture) {
            SDL_RenderCopy(renderer, texture, NULL, NULL);
        }
        
        if (rectangle.x != -1) {
            rectangle.x /= scale;
            rectangle.y /= scale;
            rectangle.w /= scale;
            rectangle.h /= scale;

            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            SDL_RenderDrawRect(renderer, &rectangle);

            rectangle.x *= scale;
            rectangle.y *= scale;
            rectangle.w *= scale;
            rectangle.h *= scale;
        }
        
        SDL_RenderPresent(renderer);
    }

    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
    
    SDL_DestroyWindow(window);
    SDL_DestroyRenderer(renderer);

    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
