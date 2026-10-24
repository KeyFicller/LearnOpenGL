using System;

namespace Scripts
{
    /// <summary>
    /// Unified CS-side input. Host calls Input.SetFrame once per frame directly;
    /// game scripts (e.g. ExampleScript.Update) only read Move/Mouse/Buttons here.
    /// </summary>
    public static class Input
    {
        public static float MoveX { get; private set; }
        public static float MoveY { get; private set; }
        public static float MouseX { get; private set; }
        public static float MouseY { get; private set; }

        /// <summary>bit0 left, bit1 right, bit2 Space — held this frame.</summary>
        public static int Buttons { get; private set; }
        /// <summary>Same bits — edge pressed this frame.</summary>
        public static int Clicked { get; private set; }

        public static void SetFrame(float moveX, float moveY,
                                    float mouseX, float mouseY,
                                    int buttons, int clicked)
        {
            MoveX = moveX;
            MoveY = moveY;
            MouseX = mouseX;
            MouseY = mouseY;
            Buttons = buttons;
            Clicked = clicked;
        }

        public static bool ButtonHeld(int bit) { return (Buttons & (1 << bit)) != 0; }
        public static bool ButtonDown(int bit) { return (Clicked & (1 << bit)) != 0; }
    }
}
