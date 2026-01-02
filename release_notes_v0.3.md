# Starboy v0.3 — Planned work

This release will address the next five issues currently in the repository's "Ready" state.

Planned items (short):

1. Issue #2 — "collision with asteroids"
   - Improve collision detection and handling between ship and asteroids.
   - Reset / respawn behavior for the ship and optionally destroy or split asteroids.

2. Issue #3 — "Level system"
   - Add a simple level/score progression system with thresholds and a small level-up effect.
   - Use levels to increase difficulty (asteroid count or speed) or unlock features.

3. Issue #4 — "Add pickups"
   - Implement pickup items that randomly spawn (guns, boosters, shields).
   - Simple pickup collection logic and temporary effect application.

4. Issue #5 — "collision effects"
   - Add visual effects (sparks, small explosions) and brief screen flash on collisions.
   - Add hooks for sound playback when available.

5. Issue #8 — "A start/restart should randomize asteroids"
   - Ensure restart creates random asteroid positions, sizes and shapes so runs vary.


Development plan:
- Implement each item incrementally on `release/v0.3` with small commits.
- Add feature toggles where appropriate and persist important settings to `starboy_settings.txt`.
- Test by building and running locally with `scripts\build_and_run.ps1`.

When complete I'll merge `release/v0.3` back to `main` and publish `v0.3` release notes.
