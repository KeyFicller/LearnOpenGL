using System;

namespace Scripts
{
    /// <summary>
    /// Entry points the Script Editor scene invokes from C++.
    /// OnUpdate + OffsetX/Y form the teachable host → script → render loop;
    /// motion math lives in MoveController (same Scripts.dll).
    /// </summary>
    public static class ExampleScript
    {
        // Avoid Console — System.Native DllNotFound under embedded Mono.
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

        /// <summary>Frame tick from C++ (delta seconds).</summary>
        public static void OnUpdate(float deltaTime)
        {
            MoveController.OnUpdate(deltaTime);
        }

        /// <summary>Forward path radius to MoveController (ImGui / host).</summary>
        public static void SetPathRadius(float radius)
        {
            MoveController.setPathRadius(radius);
        }

        public static float OffsetX()
        {
            return MoveController.MoveX();
        }

        public static float OffsetY()
        {
            return MoveController.MoveY();
        }
    }
}
