using System;

namespace Scripts
{
    /// <summary>
    /// Circular path helper for Spline Movement "Driven by Script".
    /// Script Editor mini-game lives in ExampleScript (HandleInput / OnUpdate).
    /// MoveX/MoveY take absolute time (e.g. glfwGetTime), not frame dt.
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
