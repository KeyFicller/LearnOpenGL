using System;

namespace Scripts
{
    /// <summary>
    /// Example C# script invoked from C++ Mono host.
    /// </summary>
    public static class MoveController
    {
        public static float Radius { get; private set; } = 0.8f;

        public static void setPathRadius(float radius)
        {
            Radius = radius;
        }

        public static float MoveX(float deltaTime)
        {
            return Radius * (float)Math.Cos(deltaTime);
        }

        public static float MoveY(float deltaTime)
        {
            return Radius * (float)Math.Sin(deltaTime);
        }
    }
}
