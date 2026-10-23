using System;

namespace Scripts
{
    /// <summary>
    /// Circular path driven by the Mono host frame loop.
    /// C++ calls OnUpdate(deltaTime) each frame, then MoveX/MoveY for uOffset.
    /// </summary>
    public static class MoveController
    {
        public static float Radius { get; private set; } = 0.45f;

        static float s_elapsed;

        public static float Elapsed => s_elapsed;

        public static void setPathRadius(float radius)
        {
            if (radius < 0.05f) radius = 0.05f;
            if (radius > 1.0f) radius = 1.0f;
            Radius = radius;
        }

        /// <summary>Accumulate elapsed seconds from the host frame delta.</summary>
        public static void OnUpdate(float deltaTime)
        {
            if (deltaTime > 0.0f)
                s_elapsed += deltaTime;
        }

        public static float MoveX()
        {
            return Radius * (float)Math.Cos(s_elapsed);
        }

        public static float MoveY()
        {
            return Radius * (float)Math.Sin(s_elapsed);
        }
    }
}
