using System;

namespace Scripts
{
    /// <summary>
    /// Minimal collect demo for Script Editor teaching: edit this file, Save,
    /// (macOS) hot-reload. CS owns pose/score; host only records input and draws.
    ///
    /// Unity-like lifecycle: host calls Start once (load/reset), then each frame
    /// Input.SetFrame + Time.SetFrame + Update. Scripts only *read* Input / Time.
    ///
    /// Scriptable fields below are the "serialized" surface: host ImGui inspector
    /// reads/writes them live, OR edit defaults here and Save to hot-reload.
    /// </summary>
    public static class ExampleScript
    {
        // --- Scriptable / inspector fields (defaults; host can tweak at runtime) ---
        public static float MoveSpeed = 1.1f;
        public static float PlayerHalf = 0.12f;
        public static float TargetHalf = 0.08f;
        public static int GoalScore = 5;
        public static float PlayerR = 0.95f, PlayerG = 0.35f, PlayerB = 0.3f;
        public static float TargetR = 0.2f, TargetG = 0.85f, TargetB = 0.9f;

        const float Arena = 0.85f;

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
            float speed = MoveSpeed;
            float pHalf = PlayerHalf;
            float tHalf = TargetHalf;

            s_playerX += Input.MoveX * speed * dt;
            s_playerY += Input.MoveY * speed * dt;
            s_playerX = Clamp(s_playerX, -Arena, Arena);
            s_playerY = Clamp(s_playerY, -Arena, Arena);

            bool collect = OverlapsTarget(s_playerX, s_playerY, pHalf, tHalf);
            if (Input.ButtonDown(0) &&
                OverlapsPoint(s_targetX, s_targetY, tHalf + 0.05f, Input.MouseX, Input.MouseY))
            {
                collect = true;
            }

            if (collect)
            {
                s_score += 1;
                if (s_score >= GoalScore)
                    s_won = true;
                else
                    RespawnTarget();
            }
        }

        // --- Pose / HUD pull (host render) ---
        public static float PlayerX() { return s_playerX; }
        public static float PlayerY() { return s_playerY; }
        public static float TargetX() { return s_targetX; }
        public static float TargetY() { return s_targetY; }
        public static int Score() { return s_score; }
        public static int IsWon() { return s_won ? 1 : 0; }

        // --- Scriptable field get/set (host ImGui inspector) ---
        public static float GetMoveSpeed() { return MoveSpeed; }
        public static void SetMoveSpeed(float v) { MoveSpeed = Clamp(v, 0.05f, 8.0f); }

        public static float GetPlayerHalf() { return PlayerHalf; }
        public static void SetPlayerHalf(float v) { PlayerHalf = Clamp(v, 0.04f, 0.4f); }

        public static float GetTargetHalf() { return TargetHalf; }
        public static void SetTargetHalf(float v) { TargetHalf = Clamp(v, 0.03f, 0.3f); }

        public static int GetGoalScore() { return GoalScore; }
        public static void SetGoalScore(int v)
        {
            if (v < 1) v = 1;
            if (v > 50) v = 50;
            GoalScore = v;
        }

        public static float GetPlayerR() { return PlayerR; }
        public static float GetPlayerG() { return PlayerG; }
        public static float GetPlayerB() { return PlayerB; }
        public static void SetPlayerColor(float r, float g, float b)
        {
            PlayerR = Clamp01(r);
            PlayerG = Clamp01(g);
            PlayerB = Clamp01(b);
        }

        public static float GetTargetR() { return TargetR; }
        public static float GetTargetG() { return TargetG; }
        public static float GetTargetB() { return TargetB; }
        public static void SetTargetColor(float r, float g, float b)
        {
            TargetR = Clamp01(r);
            TargetG = Clamp01(g);
            TargetB = Clamp01(b);
        }

        static void RespawnTarget()
        {
            float tHalf = TargetHalf;
            float pHalf = PlayerHalf;
            for (int i = 0; i < 12; ++i)
            {
                float x = (float)(s_rng.NextDouble() * 2.0 - 1.0) * (Arena - tHalf);
                float y = (float)(s_rng.NextDouble() * 2.0 - 1.0) * (Arena - tHalf);
                if (!OverlapsPoint(s_playerX, s_playerY, pHalf + tHalf + 0.15f, x, y))
                {
                    s_targetX = x;
                    s_targetY = y;
                    return;
                }
            }
            s_targetX = -s_playerX * 0.5f;
            s_targetY = -s_playerY * 0.5f;
        }

        static bool OverlapsTarget(float x, float y, float pHalf, float tHalf)
        {
            float dx = x - s_targetX;
            float dy = y - s_targetY;
            float r = pHalf + tHalf;
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

        static float Clamp01(float v)
        {
            return Clamp(v, 0.0f, 1.0f);
        }
    }
}
