#include <SDL.h>
#include <vector>
#include <cmath>
#include <chrono>
#include <random>
#include <cstring>
#if defined(__has_include)
#  if __has_include(<SDL_ttf.h>)
#    include <SDL_ttf.h>
#    define HAVE_SDL_TTF 1
#  endif
#endif
/* Forward-declare TTF_Font when SDL_ttf isn't available so pointers compile. */
#if !defined(HAVE_SDL_TTF)
struct TTF_Font;
#endif

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

// Simple filled triangle rasterizer (scanline) for small UI/flame effects.
static void drawFilledTriangle(SDL_Renderer* r, Vec2 p0, Vec2 p1, Vec2 p2) {
    auto roundi = [](float v) { return static_cast<int>(v + 0.5f); };
    // sort by Y ascending
    if (p1.y < p0.y) std::swap(p0, p1);
    if (p2.y < p0.y) std::swap(p0, p2);
    if (p2.y < p1.y) std::swap(p1, p2);
    int y0 = roundi(p0.y), y1 = roundi(p1.y), y2 = roundi(p2.y);
    if (y0 == y2) return;
    auto interpX = [](const Vec2& a, const Vec2& b, float y)->float {
        if (b.y == a.y) return a.x;
        return a.x + (b.x - a.x) * ((y - a.y) / (b.y - a.y));
    };
    for (int y = y0; y <= y2; ++y) {
        if (y < y1) {
            float xl = interpX(p0, p1, (float)y);
            float xr = interpX(p0, p2, (float)y);
            if (xl > xr) std::swap(xl, xr);
            SDL_RenderDrawLine(r, static_cast<int>(xl), y, static_cast<int>(xr), y);
        } else {
            float xl = interpX(p1, p2, (float)y);
            float xr = interpX(p0, p2, (float)y);
            if (xl > xr) std::swap(xl, xr);
            SDL_RenderDrawLine(r, static_cast<int>(xl), y, static_cast<int>(xr), y);
        }
    }
}

struct Asteroid {
    Vec2 pos;
    std::vector<Vec2> shape; // relative
    float radius; // approximate collision radius
    Vec2 vel{0.0f, 0.0f};
};

int main(int argc, char** argv) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) return -1;
#ifdef HAVE_SDL_TTF
    if (TTF_Init() != 0) {
        // Continue without TTF if initialization fails; we'll handle missing font gracefully
    }
#endif

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

    // TTF font (optional)
#ifdef HAVE_SDL_TTF
    TTF_Font* font = nullptr;
    const char* fontCandidates[] = {
        "C:/Windows/Fonts/segui.ttf",
        "C:/Windows/Fonts/SegoeUI.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "./assets/DejaVuSans.ttf",
        NULL
    };
    const char* loadedFontPath = nullptr;
    for (int i = 0; fontCandidates[i] != NULL; ++i) {
        font = TTF_OpenFont(fontCandidates[i], 24);
        if (font) { loadedFontPath = fontCandidates[i]; break; }
    }
#else
    TTF_Font* font = nullptr; // stub when TTF not available
#endif

    // Asteroids - simple fixed ones
    std::vector<Asteroid> asts;
    // Background stars (non-colliding visual layer)
    std::vector<Vec2> stars;
    std::vector<int> starBase;
    std::vector<float> starDepth; // 0.0 = far, 1.0 = near (for parallax)
    std::vector<float> starTwinkleFreq;
    std::vector<float> starTwinklePhase;
    std::vector<float> starTwinkleAmp;

    auto createAsteroids = [&](std::vector<Asteroid>& out) {
        out.clear();
        for (int i = 0; i < 6; ++i) {
            Asteroid a;
            a.pos.x = (i + 1) * 110.0f;
            a.pos.y = 80.0f + (i % 3) * 160;
            int verts = 6 + (i % 3);
            float rradius = 30.0f + (i % 4) * 10.0f;
            a.shape.reserve(verts);
            float maxr = 0.0f;
            for (int v = 0; v < verts; ++v) {
                float ang = static_cast<float>(v) / verts * 2.0f * 3.14159265f;
                float rr = rradius * (0.8f + 0.4f * std::sin(v * 1.3f + i));
                float px = std::cos(ang) * rr;
                float py = std::sin(ang) * rr;
                a.shape.push_back({ px, py });
                float len = std::sqrt(px*px + py*py);
                if (len > maxr) maxr = len;
            }
            a.radius = maxr;
            // initial velocity (small drift) - deterministic based on index
            a.vel.x = (i % 2 == 0) ? 8.0f : -6.0f;
            a.vel.y = ((i % 3) - 1) * 4.0f;
            out.push_back(std::move(a));
        }
    };

        // Split an asteroid into two smaller ones (deterministic, small offsets)
        auto splitAsteroid = [&](const Asteroid &src, std::vector<Asteroid> &out) {
            // only split large asteroids
            if (src.radius < 18.0f) return;
            // produce two children with different scales and small offsets
            const float scales[2] = {0.6f, 0.5f};
            const float offsets[2][2] = {{-8.0f, -6.0f}, {8.0f, 6.0f}};
            for (int i = 0; i < 2; ++i) {
                Asteroid b;
                b.pos.x = src.pos.x + offsets[i][0];
                b.pos.y = src.pos.y + offsets[i][1];
                float maxr = 0.0f;
                for (const auto &p : src.shape) {
                    float px = p.x * scales[i];
                    float py = p.y * scales[i];
                    b.shape.push_back({px, py});
                    float len = std::sqrt(px*px + py*py);
                    if (len > maxr) maxr = len;
                }
                b.radius = maxr;
                out.push_back(std::move(b));
            }
        };

    createAsteroids(asts);

    // generate a deterministic star field (small, non-colliding background)
    const int STAR_COUNT = 140;
    {
        // fixed-seed PRNG for repeatability and even distribution
        std::mt19937 rng(1234567);
        std::uniform_real_distribution<float> rx(0.0f, static_cast<float>(W));
        std::uniform_real_distribution<float> ry(0.0f, static_cast<float>(H));
        std::uniform_real_distribution<float> rz(0.0f, 1.0f);
        std::uniform_real_distribution<float> rfreq(0.6f, 3.2f);
        std::uniform_real_distribution<float> ramp(0.12f, 0.68f);
        std::uniform_real_distribution<float> rphase(0.0f, 2.0f * 3.14159265f);
        stars.clear();
        starBase.clear();
        starDepth.clear();
        starTwinkleFreq.clear();
        starTwinklePhase.clear();
        starTwinkleAmp.clear();
        for (int i = 0; i < STAR_COUNT; ++i) {
            // world position
            stars.push_back({ rx(rng), ry(rng) });
            // small variety in apparent size
            starBase.push_back((i % 3) + 1);
            // depth: bias towards farther stars (square the random)
            float z = rz(rng);
            float depth = z * z; // bias towards farther stars
            starDepth.push_back(depth);
            // twinkle parameters: frequency, phase, amplitude (scaled by depth so nearer stars can have stronger twinkle)
            float freq = rfreq(rng);
            float phase = rphase(rng);
            float amp = ramp(rng) * (0.4f + 0.6f * depth);
            starTwinkleFreq.push_back(freq);
            starTwinklePhase.push_back(phase);
            starTwinkleAmp.push_back(amp);
        }
    }

    auto restartGame = [&](void) {
        shipPos = { static_cast<float>(W) / 2.0f, static_cast<float>(H) / 2.0f };
        shipAngle = 0.0f;
        shipVel = { 0.0f, 0.0f };
        createAsteroids(asts);
    };

    // Menu state
    bool menuOpen = false;
    std::vector<const char*> menuItems = { "Resume", "Restart", "Quit" };
    int menuSelection = 0;
    // Debug: toggle stronger twinkle so it's visible during testing
    bool starTwinkleDebug = false;

    auto renderText = [&](SDL_Renderer* r, TTF_Font* f, const char* text, SDL_Color col, int& outW, int& outH)->SDL_Texture* {
        outW = outH = 0;
#ifdef HAVE_SDL_TTF
        if (!f) return nullptr;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(f, text, col);
        if (!surf) return nullptr;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surf);
        outW = surf->w; outH = surf->h;
        SDL_FreeSurface(surf);
        return tex;
#else
        (void)r; (void)f; (void)text; (void)col;
        return nullptr;
#endif
    };

    auto last = std::chrono::high_resolution_clock::now();
    // collision / gameplay state
    const float shipRadius = 14.0f; // used for simple collision test
    float collisionFlash = 0.0f; // seconds to show collision flash
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

            if (ev.type == SDL_MOUSEBUTTONDOWN) {
                // menu icon area (top-left 40x24)
                int mx = ev.button.x;
                int my = ev.button.y;
                if (mx >= 6 && mx <= 34 && my >= 6 && my <= 18) {
                    menuOpen = true;
                    menuSelection = 0;
                } else if (menuOpen) {
                    // compute menu box geometry to detect clicks on items
                    int menuW = 320;
                    int menuH = 180;
                    int bx = (W - menuW) / 2;
                    int by = (H - menuH) / 2;
                    int padding = 18;
                    int itemH = 40;
                    if (mx >= bx && mx <= bx + menuW && my >= by && my <= by + menuH) {
                        int relativeY = my - (by + padding);
                        if (relativeY >= 0) {
                            int slot = relativeY / (itemH + 6);
                            if (slot >= 0 && slot < (int)menuItems.size()) {
                                const char* sel = menuItems[slot];
                                if (strcmp(sel, "Resume") == 0) { menuOpen = false; }
                                else if (strcmp(sel, "Restart") == 0) { restartGame(); menuOpen = false; }
                                else if (strcmp(sel, "Quit") == 0) { running = false; }
                            }
                        }
                    }
                }
            }

            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    menuOpen = !menuOpen;
                    menuSelection = 0;
                }
                // debug: toggle twinkle boost
                if (ev.key.keysym.sym == SDLK_t) {
                    starTwinkleDebug = !starTwinkleDebug;
                }
                if (menuOpen) {
                    if (ev.key.keysym.sym == SDLK_UP) {
                        menuSelection = (menuSelection - 1 + (int)menuItems.size()) % (int)menuItems.size();
                    } else if (ev.key.keysym.sym == SDLK_DOWN) {
                        menuSelection = (menuSelection + 1) % (int)menuItems.size();
                    } else if (ev.key.keysym.sym == SDLK_RETURN || ev.key.keysym.sym == SDLK_KP_ENTER) {
                        const char* sel = menuItems[menuSelection];
                        if (strcmp(sel, "Resume") == 0) { menuOpen = false; }
                        else if (strcmp(sel, "Restart") == 0) { restartGame(); menuOpen = false; }
                        else if (strcmp(sel, "Quit") == 0) { running = false; }
                    }
                }
                // quick keyboard shortcuts
                if (ev.key.keysym.sym == SDLK_r) restartGame();
                if (ev.key.keysym.sym == SDLK_q) running = false;
            }
        }

        const Uint8* k = SDL_GetKeyboardState(NULL);
        if (k[SDL_SCANCODE_LEFT]) shipAngle -= 3.0f * dt;
        if (k[SDL_SCANCODE_RIGHT]) shipAngle += 3.0f * dt;
        if (k[SDL_SCANCODE_UP]) {
            float thrust = 200.0f * dt;
            // forward vector for local (0,-1) after rotation by shipAngle:
            float fx = std::sin(shipAngle);
            float fy = -std::cos(shipAngle);
            shipVel.x += fx * thrust;
            shipVel.y += fy * thrust;
        }

        // Drag
        shipVel.x *= 0.995f;
        shipVel.y *= 0.995f;

        shipPos.x += shipVel.x * dt;
        shipPos.y += shipVel.y * dt;
        shipPos.x = wrap(shipPos.x, 0.0f, static_cast<float>(W));
        shipPos.y = wrap(shipPos.y, 0.0f, static_cast<float>(H));

        // Simple collision detection: ship vs asteroid (circle-circle approx)
        for (int i = static_cast<int>(asts.size()) - 1; i >= 0; --i) {
            const Asteroid a = asts[i];
            float dx = shipPos.x - a.pos.x;
            float dy = shipPos.y - a.pos.y;
            float dist2 = dx*dx + dy*dy;
            float r = shipRadius + a.radius;
            if (dist2 <= r * r) {
                // Collision occurred: split asteroid if large enough, otherwise remove
                std::vector<Asteroid> children;
                splitAsteroid(a, children);
                // erase the original
                asts.erase(asts.begin() + i);
                // if children produced, set small velocities for them
                for (size_t ci = 0; ci < children.size(); ++ci) {
                    // velocity roughly perpendicular to offset direction
                    float vx = (ci == 0) ? -40.0f : 40.0f;
                    float vy = (ci == 0) ? -24.0f : 24.0f;
                    children[ci].vel.x = vx;
                    children[ci].vel.y = vy;
                    asts.push_back(std::move(children[ci]));
                }
                // reset ship
                shipPos = { static_cast<float>(W) / 2.0f, static_cast<float>(H) / 2.0f };
                shipVel = { 0.0f, 0.0f };
                collisionFlash = 0.6f;
                break; // handle one collision per frame
            }
        }

        // Render
        SDL_SetRenderDrawColor(ren, 8, 8, 20, 255);
        SDL_RenderClear(ren);

        // Update and draw asteroids
        // Draw background stars with simple parallax layers
        if (!stars.empty()) {
            float tt = SDL_GetTicks() * 0.001f;
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            // camera offset (world -> screen) based on ship centered in screen
            Vec2 cam{ shipPos.x - static_cast<float>(W) / 2.0f, shipPos.y - static_cast<float>(H) / 2.0f };
            for (size_t si = 0; si < stars.size(); ++si) {
                float depth = (si < starDepth.size()) ? starDepth[si] : 0.0f; // 0..1, 0 = far
                float par = 1.0f - depth; // how strongly the star follows the camera
                float sx = stars[si].x - cam.x * par;
                float sy = stars[si].y - cam.y * par;
                sx = wrap(sx, 0.0f, static_cast<float>(W));
                sy = wrap(sy, 0.0f, static_cast<float>(H));

                // twinkle computed per-star using precomputed freq/phase/amp
                float freq = (si < starTwinkleFreq.size()) ? starTwinkleFreq[si] : (0.8f + (si % 3) * 0.2f);
                float phase = (si < starTwinklePhase.size()) ? starTwinklePhase[si] : 0.0f;
                float amp = (si < starTwinkleAmp.size()) ? starTwinkleAmp[si] : 0.25f;
                float tw = 0.5f + 0.5f * std::sin(tt * freq + phase); // 0..1
                float debugBoost = starTwinkleDebug ? 3.0f : 1.0f;
                float flick = 0.6f + 0.4f * (0.6f * tw + 0.4f * (1.0f - depth)) * (1.0f + amp * 0.8f * debugBoost);
                int base = starBase[si % starBase.size()];
                int alpha = static_cast<int>(40 + 120 * flick * (0.4f + 0.6f * depth));
                Uint8 col = static_cast<Uint8>(200 * (0.6f + 0.4f * depth));
                SDL_SetRenderDrawColor(ren, col, col, 230, alpha);

                int size = base + ((depth > 0.8f) ? 1 : 0);
                if (size <= 1) {
                    SDL_RenderDrawPoint(ren, static_cast<int>(sx), static_cast<int>(sy));
                } else {
                    SDL_Rect r{ static_cast<int>(sx) - size/2, static_cast<int>(sy) - size/2, size, size };
                    SDL_RenderFillRect(ren, &r);
                }
            }
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }

        // small debug indicator for twinkle boost (top-right)
        if (starTwinkleDebug) {
            SDL_SetRenderDrawColor(ren, 60, 200, 80, 255);
        } else {
            SDL_SetRenderDrawColor(ren, 80, 80, 80, 255);
        }
        SDL_Rect dbg{ W - 18, 6, 12, 12 };
        SDL_RenderFillRect(ren, &dbg);

        // Draw asteroids
        SDL_SetRenderDrawColor(ren, 180, 180, 160, 255);
        for (auto &a : asts) {
            // update position
            a.pos.x += a.vel.x * dt;
            a.pos.y += a.vel.y * dt;
            a.pos.x = wrap(a.pos.x, 0.0f, static_cast<float>(W));
            a.pos.y = wrap(a.pos.y, 0.0f, static_cast<float>(H));
            std::vector<Vec2> absPts;
            absPts.reserve(a.shape.size());
            for (const auto& p : a.shape) {
                absPts.push_back({p.x + a.pos.x, p.y + a.pos.y});
            }
            drawPolygon(ren, absPts);
        }

        // collision flash overlay (brief)
        if (collisionFlash > 0.0f) {
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            int alpha = static_cast<int>(std::min(255.0f, collisionFlash / 0.6f * 220.0f));
            SDL_SetRenderDrawColor(ren, 220, 60, 60, alpha);
            SDL_Rect full{0,0,W,H};
            SDL_RenderFillRect(ren, &full);
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
            // decrease timer
            collisionFlash -= dt;
            if (collisionFlash < 0.0f) collisionFlash = 0.0f;
        }

        // Draw a small menu icon (top-left)
        SDL_SetRenderDrawColor(ren, 120, 120, 140, 255);
        SDL_Rect menuRect{6, 6, 28, 12};
        SDL_RenderFillRect(ren, &menuRect);
        // draw hamburger lines
        SDL_SetRenderDrawColor(ren, 200, 200, 220, 255);
        for (int i = 0; i < 3; ++i) {
            int y = 8 + i * 4;
            SDL_RenderDrawLine(ren, 10, y, 30, y);
        }

        // Draw ship as a simple starship shape (nose + wings + rear)
        SDL_SetRenderDrawColor(ren, 220, 220, 255, 255);
        std::vector<Vec2> shipPts;
        float sr = 14.0f;
        // Define local points (nose-up coordinate system)
        std::vector<Vec2> local = {
            { 0.0f, -sr * 1.6f }, // nose
            { -sr * 0.6f, -sr * 0.3f }, // left upper
            { -sr * 1.2f,  sr * 0.8f }, // left wing tip
            { -sr * 0.3f,  sr * 0.6f }, // left rear inner
            {  sr * 0.3f,  sr * 0.6f }, // right rear inner
            {  sr * 1.2f,  sr * 0.8f }, // right wing tip
            {  sr * 0.6f, -sr * 0.3f }  // right upper
        };

        // Rotate and translate local points into world space.
        // Use the same `shipAngle` as the rotation so nose and thrust align.
        float rot = shipAngle;
        float cr = std::cos(rot);
        float srn = std::sin(rot);
        shipPts.reserve(local.size());
        for (const auto& p : local) {
            float x = cr * p.x - srn * p.y + shipPos.x;
            float y = srn * p.x + cr * p.y + shipPos.y;
            shipPts.push_back({ x, y });
        }

        // Thrust flame (draw behind the ship when UP is pressed)
        bool thrusting = k[SDL_SCANCODE_UP];
        if (thrusting) {
            float t = SDL_GetTicks() * 0.001f;
            float flick = (std::sin(t * 30.0f) * 0.5f + 0.5f) * 6.0f;
            std::vector<Vec2> flameLocal = {
                { -sr * 0.5f,  sr * 0.9f },
                {  0.0f,       sr * 1.6f + flick },
                {  sr * 0.5f,  sr * 0.9f }
            };
            std::vector<Vec2> flamePts;
            flamePts.reserve(flameLocal.size());
            for (const auto& p : flameLocal) {
                float x = cr * p.x - srn * p.y + shipPos.x;
                float y = srn * p.x + cr * p.y + shipPos.y;
                flamePts.push_back({ x, y });
            }
            // outer glow
            SDL_SetRenderDrawColor(ren, 255, 120, 20, 255);
            drawFilledTriangle(ren, flamePts[1], flamePts[0], flamePts[2]);
            // inner core (smaller, brighter)
            std::vector<Vec2> corePts;
            corePts.reserve(3);
            for (const auto& fp : flamePts) {
                corePts.push_back({ shipPos.x + (fp.x - shipPos.x) * 0.5f, shipPos.y + (fp.y - shipPos.y) * 0.5f });
            }
            SDL_SetRenderDrawColor(ren, 255, 220, 40, 255);
            drawFilledTriangle(ren, corePts[1], corePts[0], corePts[2]);
        }

        drawPolygon(ren, shipPts);

        // If menu is open, render overlay and menu items on top
        if (menuOpen) {
            // Render overlay
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 160);
            SDL_Rect full{0,0,W,H};
            SDL_RenderFillRect(ren, &full);

            int menuW = 320;
            int menuH = 180;
            int mx = (W - menuW) / 2;
            int my = (H - menuH) / 2;
            SDL_SetRenderDrawColor(ren, 30, 30, 40, 220);
            SDL_Rect box{mx, my, menuW, menuH};
            SDL_RenderFillRect(ren, &box);

            // Draw items
            int padding = 18;
            int itemH = 40;
            SDL_Color white{240,240,240,255};
            SDL_Color yellow{255,220,40,255};
            for (size_t i = 0; i < menuItems.size(); ++i) {
                int ix = mx + padding;
                int iy = my + padding + static_cast<int>(i) * (itemH + 6);
                // highlight
                if ((int)i == menuSelection) {
                    SDL_SetRenderDrawColor(ren, 60, 60, 80, 200);
                    SDL_Rect h{ix-8, iy-6, menuW - padding*2 + 16, itemH+8};
                    SDL_RenderFillRect(ren, &h);
                }
                // draw a small arrow indicator for selection (even without font)
                if ((int)i == menuSelection) {
                    SDL_SetRenderDrawColor(ren, 255, 220, 40, 255);
                    SDL_RenderDrawLine(ren, ix - 14, iy + itemH/2, ix - 6, iy + itemH/2 - 6);
                    SDL_RenderDrawLine(ren, ix - 14, iy + itemH/2, ix - 6, iy + itemH/2 + 6);
                }
                // render text if font available
                int tw = 0, th = 0;
                SDL_Texture* txt = renderText(ren, font, menuItems[i], white, tw, th);
                if (txt) {
                    SDL_Rect dst{ ix + 10, iy + (itemH - th)/2, tw, th };
                    if ((int)i == menuSelection) {
                        // draw selected in yellow
                        SDL_DestroyTexture(txt);
                        txt = renderText(ren, font, menuItems[i], yellow, tw, th);
                        dst.w = tw; dst.h = th;
                    }
                    SDL_RenderCopy(ren, txt, NULL, &dst);
                    SDL_DestroyTexture(txt);
                } else {
                    // fallback: draw label rectangle
                    SDL_SetRenderDrawColor(ren, 120, 120, 140, 255);
                    SDL_Rect lbl{ ix+10, iy + (itemH/4), 140, itemH/2 };
                    SDL_RenderFillRect(ren, &lbl);
                }
            }

            // reset blend mode
            SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        }

        SDL_RenderPresent(ren);

        // Cap ~60fps
        SDL_Delay(16);
    }

#ifdef HAVE_SDL_TTF
    if (font) TTF_CloseFont(font);
    TTF_Quit();
#endif
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
