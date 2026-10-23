using System;

namespace Scripts
{
    /// <summary>
    /// Mini-game owned by this C# script. The C++ host only records input
    /// (script_input_content) and renders quads from the properties below.
    ///
    /// Lifecycle each frame:
    ///   1. HandleInput(...)  — receive axes / mouse / buttons from the host
    ///   2. OnUpdate(dt)      — move, collect, score (game logic lives here)
    ///   3. Host reads PlayerX/Y, TargetX/Y, Score, IsWon and draws
    ///
    /// HandleInput contract:
    ///   moveX/Y     : WASD/arrows axis in [-1, 1] (host zeroes when ImGui wants keyboard)
    ///   mouseX/Y    : cursor in clip NDC [-1, 1], y up (viewport-relative)
    ///   mouseButtons: bit0 = left held, bit1 = right held
    ///   mouseClicked: bit0 = left pressed this frame, bit1 = right pressed this frame
    /// </summary>
    public static class ExampleScript
    {
        const float PlayerSpeed = 1.1f;
        const float PlayerHalf = 0.12f;
        const float TargetHalf = 0.08f;
        const float Arena = 0.85f;
        const int WinScore = 5;

        // --- Input content (written by HandleInput, consumed by OnUpdate) ---
        static float s_moveX, s_moveY;
        static float s_mouseX, s_mouseY;
        static int s_mouseButtons, s_mouseClicked;

        // --- Game state (authoritative; host only mirrors for rendering) ---
        static float s_playerX, s_playerY;
        static float s_targetX = 0.45f, s_targetY = 0.35f;
        static int s_score;
        static bool s_won;
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

        /// <summary>Per-frame game tick: movement, collect, win check.</summary>
        public static void OnUpdate(float deltaTime)
        {
            if (deltaTime < 0.0f)
                deltaTime = 0.0f;
            if (deltaTime > 0.1f)
                deltaTime = 0.1f;

            if (s_won)
                return;

            s_playerX += s_moveX * PlayerSpeed * deltaTime;
            s_playerY += s_moveY * PlayerSpeed * deltaTime;
            s_playerX = Clamp(s_playerX, -Arena, Arena);
            s_playerY = Clamp(s_playerY, -Arena, Arena);

            bool collect = OverlapsTarget(s_playerX, s_playerY);
            // Left click near target also collects (mouse aim)
            if ((s_mouseClicked & 1) != 0 &&
                OverlapsPoint(s_targetX, s_targetY, TargetHalf + 0.05f, s_mouseX, s_mouseY))
            {
                collect = true;
            }

            if (collect)
            {
                s_score += 1;
                if (s_score >= WinScore)
                {
                    s_won = true;
                }
                else
                {
                    RespawnTarget();
                }
            }

            // Buttons available for script experiments beyond the mini-game.
            _ = s_mouseButtons;
        }

        /// <summary>Host calls on R / UI Reset — clear score and recenter.</summary>
        public static void ResetGame()
        {
            s_playerX = 0.0f;
            s_playerY = 0.0f;
            s_score = 0;
            s_won = false;
            RespawnTarget();
        }

        public static float PlayerX() { return s_playerX; }
        public static float PlayerY() { return s_playerY; }
        public static float TargetX() { return s_targetX; }
        public static float TargetY() { return s_targetY; }
        public static int Score() { return s_score; }
        public static int IsWon() { return s_won ? 1 : 0; }

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
                    return;
                }
            }
            s_targetX = -s_playerX * 0.5f;
            s_targetY = -s_playerY * 0.5f;
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
