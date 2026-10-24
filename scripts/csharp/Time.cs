namespace Scripts
{
    /// <summary>
    /// Unified CS-side clock. Host passes dt into ExampleScript.OnUpdate;
    /// that forwards into SetFrame. Scripts read DeltaTime / Time since start here.
    /// </summary>
    public static class Time
    {
        public static float DeltaTime { get; private set; }
        public static float TimeSinceStartup { get; private set; }

        public static void SetFrame(float deltaTime)
        {
            if (deltaTime < 0.0f)
                deltaTime = 0.0f;
            if (deltaTime > 0.1f)
                deltaTime = 0.1f;
            DeltaTime = deltaTime;
            TimeSinceStartup += deltaTime;
        }

        public static void Reset()
        {
            DeltaTime = 0.0f;
            TimeSinceStartup = 0.0f;
        }
    }
}
