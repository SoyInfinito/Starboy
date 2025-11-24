#include <SDL.h>
#include <vector>
#include <cmath>
#include <chrono>

struct Vec2 {
    float x;
    float y;
};

static float wrap(float v, float a, float b) {
    float w = b - a;
    while (v < a) v += w;
    while (v >= b) v -= w;
    return v;
}

void drawPolygon(SDL_Renderer* r, const std::vector<Vec2>& pts, int tx = 0, int ty = 0) {
    if (pts.size() < 2) return;
    std::vector<SDL_Point> spts(pts.size() + 1);
    for (size_t i = 0; i < pts.size(); ++i) {
        spts[i].x = static_cast<int>(pts[i].x + tx);
        spts[i].y = static_cast<int>(pts[i].y + ty);
    }
    spts[pts.size()].x = static_cast<int>(pts[0].x + tx);
    spts[pts.size()].y = static_cast<int>(pts[0].y + ty);
    SDL_RenderDrawLines(r, spts.data(), static_cast<int>(spts.size()));
}

struct Asteroid {
    Vec2 pos;
    std::vector<Vec2> shape; // relative
};

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return -1;

    const int W = 800, H = 600;
    SDL_Window* win = SDL_CreateWindow("Starboy - prototype",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        W, H, SDL_WINDOW_SHOWN);
    if (!win) return -1;
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    if (!ren) return -1;

    // Ship
    Vec2 shipPos{ static_cast<float>(W) / 2.0f, static_cast<float>(H) / 2.0f };
    float shipAngle = 0.0f; // radians
    Vec2 shipVel{ 0.0f, 0.0f };

    // Asteroids - simple fixed ones
    std::vector<Asteroid> asts;
    for (int i = 0; i < 6; ++i) {
        Asteroid a;
        a.pos.x = (i + 1) * 110.0f;
        a.pos.y = 80.0f + (i % 3) * 160;
        int verts = 6 + (i % 3);
        float rradius = 30.0f + (i % 4) * 10.0f;
        for (int v = 0; v < verts; ++v) {
            float ang = static_cast<float>(v) / verts * 2.0f * 3.14159265f;
            float rr = rradius * (0.8f + 0.4f * std::sin(v * 1.3f + i));
            a.shape.push_back({ std::cos(ang) * rr, std::sin(ang) * rr });
        }
        asts.push_back(std::move(a));
    }

    auto last = std::chrono::high_resolution_clock::now();
    bool running = true;
    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> dtf = now - last;
        last = now;
        float dt = dtf.count();
        if (dt > 0.05f) dt = 0.05f;

        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN && ev.key.keysym.sym == SDLK_ESCAPE) running = false;
        }

        const Uint8* k = SDL_GetKeyboardState(NULL);
        if (k[SDL_SCANCODE_LEFT]) shipAngle -= 3.0f * dt;
        if (k[SDL_SCANCODE_RIGHT]) shipAngle += 3.0f * dt;
        if (k[SDL_SCANCODE_UP]) {
            float thrust = 200.0f * dt;
            shipVel.x += std::cos(shipAngle - 3.14159265f / 2.0f) * thrust;
            shipVel.y += std::sin(shipAngle - 3.14159265f / 2.0f) * thrust;
        }

        // Drag
        shipVel.x *= 0.995f;
        shipVel.y *= 0.995f;

        shipPos.x += shipVel.x * dt;
        shipPos.y += shipVel.y * dt;
        shipPos.x = wrap(shipPos.x, 0.0f, static_cast<float>(W));
        shipPos.y = wrap(shipPos.y, 0.0f, static_cast<float>(H));

        // Render
        SDL_SetRenderDrawColor(ren, 8, 8, 20, 255);
        SDL_RenderClear(ren);

        // Draw asteroids
        SDL_SetRenderDrawColor(ren, 180, 180, 160, 255);
        for (auto& a : asts) {
            std::vector<Vec2> rel;
            rel.reserve(a.shape.size());
            for (auto& p : a.shape) rel.push_back({ p.x + a.pos.x, p.y + a.pos.y });
            drawPolygon(ren, rel);
        }

        // Draw ship as triangle
        SDL_SetRenderDrawColor(ren, 220, 220, 255, 255);
        std::vector<Vec2> shipPts;
        float sr = 14.0f;
        for (int i = 0; i < 3; ++i) {
            float ang = shipAngle + (i == 0 ? -3.14159265f / 2.0f : 3.14159265f / 6.0f + (i - 1) * 3.14159265f / 3.0f);
            shipPts.push_back({ std::cos(ang) * sr + shipPos.x, std::sin(ang) * sr + shipPos.y });
        }
        drawPolygon(ren, shipPts);

        SDL_RenderPresent(ren);

        // Cap ~60fps
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
