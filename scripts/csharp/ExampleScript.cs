using System;

namespace Scripts
{
    /// <summary>
    /// Minimal collect demo for Script Editor teaching: edit this file, Save,
    /// (macOS) hot-reload. CS owns pose/score; host only records input and draws.
    ///
    /// Unity-like lifecycle: host calls Start once (load/reset), then each frame
    /// Input.SetFrame + Time.SetFrame + Update. Scripts only *read* Input / Time.
    /// </summary>
    public static class ExampleScript
    {
        const float PlayerSpeed = 1.1f;
        const float PlayerHalf = 0.12f;
        const float TargetHalf = 0.08f;
        const float Arena = 0.85f;
        const int WinScore = 5;

        static float s_playerX, s_playerY;
        static float s_targetX = 0.45f, s_targetY = 0.35f;
        static int s_score;
        static bool s_won;

        static readonly Random s_rng = new Random(7);

        /// <summary>Called once on load / reset (Unity Start).</summary>
        public static void Start()
        {
            s_playerX = 0.0f;
            s_playerY = 0.0f;
            s_score = 0;
            s_won = false;
            Time.Reset();
            RespawnTarget();
        }

        /// <summary>Per-frame tick (Unity Update). Reads Input + Time only.</summary>
        public static void Update()
        {
            if (s_won)
                return;

            float dt = Time.DeltaTime;
            s_playerX += Input.MoveX * PlayerSpeed * dt;
            s_playerY += Input.MoveY * PlayerSpeed * dt;
            s_playerX = Clamp(s_playerX, -Arena, Arena);
            s_playerY = Clamp(s_playerY, -Arena, Arena);

            bool collect = OverlapsTarget(s_playerX, s_playerY);
            if (Input.ButtonDown(0) &&
                OverlapsPoint(s_targetX, s_targetY, TargetHalf + 0.05f, Input.MouseX, Input.MouseY))
            {
                collect = true;
            }

            if (collect)
            {
                s_score += 1;
                if (s_score >= WinScore)
                    s_won = true;
                else
                    RespawnTarget();
            }
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
