namespace Scripts
{
    /// <summary>
    /// Unified CS-side clock. Host calls Time.SetFrame(dt) once per frame directly;
    /// game scripts (e.g. ExampleScript.Update) only read DeltaTime / TimeSinceStartup.
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
