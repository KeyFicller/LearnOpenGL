using System;

namespace Scripts
{
    /// <summary>
    /// Example C# script invoked from C++ Mono host.
    /// </summary>
    public static class ExampleScript
    {
        // Returns greeting string - avoid Console to prevent System.Native DllNotFound in embedded Mono
        public static string SayHello()
        {
            return "Hello from C# script!";
        }

        public static int SayInt()
        {
            return 123;
        }

        public static float SayFloat()
        {
            return 123.456f;
        }

        public static void OnUpdate(float deltaTime)
        {
            // Called each frame from C++ - can be used for script lifecycle
            // Console.WriteLine($"OnUpdate dt={deltaTime}");
        }

        public static float OffsetX()
        {
            return 0.5f;
        }

        public static float OffsetY()
        {
            return 0.2f;
        }
    }
}















