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
    /// Scriptable fields use [ScriptField] (SerializeField-style). Host discovers
    /// them via ScriptFieldMirror reflection — no per-field Get/Set glue in C++.
    /// Edit defaults here + Save, or tweak live in the Script Inspector.
    /// Live inspector overrides survive Save/hot-reload (matched by name+kind);
    /// host "Refresh fields from script" re-pulls these CS defaults.
    /// </summary>
    public static class ExampleScript
    {
        // --- Scriptable / inspector fields (discovered by ScriptFieldMirror) ---
        [ScriptField(0.05f, 8.0f)]
        public static float MoveSpeed = 1.1f;

        [ScriptField(0.04f, 0.4f)]
        public static float PlayerHalf = 0.12f;

        [ScriptField(0.03f, 0.3f)]
        public static float TargetHalf = 0.08f;

        [ScriptField(1f, 50f)]
        public static int GoalScore = 5;

        [ScriptField]
        public static bool InvertY = false;

        [ScriptField]
        public static Color PlayerColor = new Color(0.95f, 0.35f, 0.3f);

        [ScriptField]
        public static Color TargetColor = new Color(0.2f, 0.85f, 0.9f);

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
            float moveY = InvertY ? -Input.MoveY : Input.MoveY;

            s_playerX += Input.MoveX * speed * dt;
            s_playerY += moveY * speed * dt;
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
    }
}
