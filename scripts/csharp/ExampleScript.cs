using System;

namespace Scripts
{
    /// <summary>
    /// Mini-game owned by this C# script. The C++ host only records input
    /// (script_input_content) and renders quads from the properties below.
    ///
    /// Lifecycle each frame:
    ///   1. HandleInput(...)  — receive axes / mouse / action bits from the host
    ///   2. OnUpdate(dt)      — dash, drift target, combo, timer (game logic here)
    ///   3. Host reads Player/Target/Score/Combo/TimeLeft and draws
    ///
    /// HandleInput contract:
    ///   moveX/Y     : WASD/arrows axis in [-1, 1] (host zeroes when viewport unfocused)
    ///   mouseX/Y    : cursor in clip NDC [-1, 1], y up (viewport-relative)
    ///   mouseButtons: bit0 = left held, bit1 = right held, bit2 = Space held
    ///   mouseClicked: bit0 = left pressed this frame, bit1 = right, bit2 = Space
    ///
    /// Teaching deepen (over collect-only):
    ///   - Dash on Space or right-click (burst along last move dir, cooldown)
    ///   - Drifting target that wanders in the arena
    ///   - Combo window + round timer (collect fast or lose)
    /// </summary>
    public static class ExampleScript
    {
        const float PlayerSpeed = 1.1f;
        const float PlayerHalf = 0.12f;
        const float TargetHalf = 0.08f;
        const float Arena = 0.85f;
        const int WinScore = 8;

        const float DashSpeed = 3.6f;
        const float DashDuration = 0.12f;
        const float DashCooldown = 0.55f;

        const float TargetDrift = 0.38f;
        const float ComboWindow = 2.4f;
        const float RoundTime = 28.0f;
        const float TimeBonusOnCollect = 2.0f;

        // --- Input content (written by HandleInput, consumed by OnUpdate) ---
        static float s_moveX, s_moveY;
        static float s_mouseX, s_mouseY;
        static int s_mouseButtons, s_mouseClicked;

        // --- Game state (authoritative; host only mirrors for rendering) ---
        static float s_playerX, s_playerY;
        static float s_targetX = 0.45f, s_targetY = 0.35f;
        static float s_targetVx = 0.25f, s_targetVy = -0.18f;
        static int s_score;
        static int s_combo;
        static float s_comboTimer;
        static float s_timeLeft = RoundTime;
        static bool s_won;
        static bool s_lost;

        static float s_facingX = 1.0f, s_facingY = 0.0f;
        static float s_dashTimer;
        static float s_dashCooldown;
        static float s_dashDirX, s_dashDirY;

        static readonly Random s_rng = new Random(7);

        /// <summary>Host forwards one frame of input content before OnUpdate.</summary>
        public static void HandleInput(float moveX, float moveY,
                                       float mouseX, float mouseY,
                                       int mouseButtons, int mouseClicked)
        {
            s_moveX = moveX;
            s_moveY = moveY;
            s_mouseX = mouseX;
            s_mouseY = mouseY;
            s_mouseButtons = mouseButtons;
            s_mouseClicked = mouseClicked;
        }

        /// <summary>Per-frame game tick: dash, move, drift target, combo, timer.</summary>
        public static void OnUpdate(float deltaTime)
        {
            if (deltaTime < 0.0f)
                deltaTime = 0.0f;
            if (deltaTime > 0.1f)
                deltaTime = 0.1f;

            if (s_won || s_lost)
                return;

            UpdateFacing();
            TryStartDash();
            TickDashCooldown(deltaTime);
            MovePlayer(deltaTime);
            DriftTarget(deltaTime);
            TickCombo(deltaTime);

            s_timeLeft -= deltaTime;
            if (s_timeLeft <= 0.0f)
            {
                s_timeLeft = 0.0f;
                s_lost = true;
                return;
            }

            bool collect = OverlapsTarget(s_playerX, s_playerY);
            // Left click near target also collects (mouse aim)
            if ((s_mouseClicked & 1) != 0 &&
                OverlapsPoint(s_targetX, s_targetY, TargetHalf + 0.05f, s_mouseX, s_mouseY))
            {
                collect = true;
            }

            if (collect)
                OnCollected();

            // Held bits available for further script experiments.
            _ = s_mouseButtons;
        }

        /// <summary>Host calls on R / UI Reset — clear score and recenter.</summary>
        public static void ResetGame()
        {
            s_playerX = 0.0f;
            s_playerY = 0.0f;
            s_score = 0;
            s_combo = 0;
            s_comboTimer = 0.0f;
            s_timeLeft = RoundTime;
            s_won = false;
            s_lost = false;
            s_dashTimer = 0.0f;
            s_dashCooldown = 0.0f;
            s_facingX = 1.0f;
            s_facingY = 0.0f;
            RespawnTarget();
        }

        public static float PlayerX() { return s_playerX; }
        public static float PlayerY() { return s_playerY; }
        public static float TargetX() { return s_targetX; }
        public static float TargetY() { return s_targetY; }
        public static int Score() { return s_score; }
        public static int Combo() { return s_combo; }
        public static float TimeLeft() { return s_timeLeft; }
        public static int IsWon() { return s_won ? 1 : 0; }
        public static int IsLost() { return s_lost ? 1 : 0; }
        /// <summary>1 while dashing — host may tint the player quad.</summary>
        public static int IsDashing() { return s_dashTimer > 0.0f ? 1 : 0; }
        /// <summary>Seconds until dash is ready again (0 = ready).</summary>
        public static float DashCooldownLeft() { return s_dashCooldown; }

        static void UpdateFacing()
        {
            float len2 = s_moveX * s_moveX + s_moveY * s_moveY;
            if (len2 > 0.01f)
            {
                float inv = 1.0f / (float)Math.Sqrt(len2);
                s_facingX = s_moveX * inv;
                s_facingY = s_moveY * inv;
            }
        }

        static void TryStartDash()
        {
            if (s_dashTimer > 0.0f || s_dashCooldown > 0.0f)
                return;

            // Space (bit2) or right-click (bit1) — both already forwarded by host.
            bool dashPressed = (s_mouseClicked & 4) != 0 || (s_mouseClicked & 2) != 0;
            if (!dashPressed)
                return;

            s_dashDirX = s_facingX;
            s_dashDirY = s_facingY;
            s_dashTimer = DashDuration;
            s_dashCooldown = DashCooldown;
        }

        static void TickDashCooldown(float dt)
        {
            if (s_dashCooldown > 0.0f)
            {
                s_dashCooldown -= dt;
                if (s_dashCooldown < 0.0f)
                    s_dashCooldown = 0.0f;
            }
        }

        static void MovePlayer(float dt)
        {
            if (s_dashTimer > 0.0f)
            {
                s_playerX += s_dashDirX * DashSpeed * dt;
                s_playerY += s_dashDirY * DashSpeed * dt;
                s_dashTimer -= dt;
                if (s_dashTimer < 0.0f)
                    s_dashTimer = 0.0f;
            }
            else
            {
                s_playerX += s_moveX * PlayerSpeed * dt;
                s_playerY += s_moveY * PlayerSpeed * dt;
            }

            s_playerX = Clamp(s_playerX, -Arena, Arena);
            s_playerY = Clamp(s_playerY, -Arena, Arena);
        }

        static void DriftTarget(float dt)
        {
            s_targetX += s_targetVx * TargetDrift * dt;
            s_targetY += s_targetVy * TargetDrift * dt;

            // Bounce off arena walls so the target stays reachable.
            float lim = Arena - TargetHalf;
            if (s_targetX > lim) { s_targetX = lim; s_targetVx = -Math.Abs(s_targetVx); }
            if (s_targetX < -lim) { s_targetX = -lim; s_targetVx = Math.Abs(s_targetVx); }
            if (s_targetY > lim) { s_targetY = lim; s_targetVy = -Math.Abs(s_targetVy); }
            if (s_targetY < -lim) { s_targetY = -lim; s_targetVy = Math.Abs(s_targetVy); }

            // Mild steer so the path is not a straight ping-pong.
            s_targetVx += (float)(s_rng.NextDouble() - 0.5) * 0.4f * dt;
            s_targetVy += (float)(s_rng.NextDouble() - 0.5) * 0.4f * dt;
            float v2 = s_targetVx * s_targetVx + s_targetVy * s_targetVy;
            if (v2 > 1.0f)
            {
                float inv = 1.0f / (float)Math.Sqrt(v2);
                s_targetVx *= inv;
                s_targetVy *= inv;
            }
            else if (v2 < 0.04f)
            {
                // Nudge if nearly stopped.
                float ang = (float)(s_rng.NextDouble() * Math.PI * 2.0);
                s_targetVx = (float)Math.Cos(ang);
                s_targetVy = (float)Math.Sin(ang);
            }
        }

        static void TickCombo(float dt)
        {
            if (s_combo <= 0)
                return;
            s_comboTimer -= dt;
            if (s_comboTimer <= 0.0f)
            {
                s_combo = 0;
                s_comboTimer = 0.0f;
            }
        }

        static void OnCollected()
        {
            if (s_comboTimer > 0.0f)
                s_combo += 1;
            else
                s_combo = 1;
            s_comboTimer = ComboWindow;

            s_score += s_combo;
            s_timeLeft += TimeBonusOnCollect;
            if (s_timeLeft > RoundTime + 6.0f)
                s_timeLeft = RoundTime + 6.0f;

            if (s_score >= WinScore)
            {
                s_won = true;
                return;
            }
            RespawnTarget();
        }

        static void RespawnTarget()
        {
            for (int i = 0; i < 12; ++i)
            {
                float x = (float)(s_rng.NextDouble() * 2.0 - 1.0) * (Arena - TargetHalf);
                float y = (float)(s_rng.NextDouble() * 2.0 - 1.0) * (Arena - TargetHalf);
                if (!OverlapsPoint(s_playerX, s_playerY, PlayerHalf + TargetHalf + 0.15f, x, y))
                {
                    s_targetX = x;
                    s_targetY = y;
                    float ang = (float)(s_rng.NextDouble() * Math.PI * 2.0);
                    s_targetVx = (float)Math.Cos(ang);
                    s_targetVy = (float)Math.Sin(ang);
                    return;
                }
            }
            s_targetX = -s_playerX * 0.5f;
            s_targetY = -s_playerY * 0.5f;
            s_targetVx = 0.3f;
            s_targetVy = -0.2f;
        }

        static bool OverlapsTarget(float x, float y)
        {
            float dx = x - s_targetX;
            float dy = y - s_targetY;
            float r = PlayerHalf + TargetHalf;
            return dx * dx + dy * dy <= r * r;
        }

        static bool OverlapsPoint(float cx, float cy, float radius, float px, float py)
        {
            float dx = cx - px;
            float dy = cy - py;
            return dx * dx + dy * dy <= radius * radius;
        }

        static float Clamp(float v, float lo, float hi)
        {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        }
    }
}
