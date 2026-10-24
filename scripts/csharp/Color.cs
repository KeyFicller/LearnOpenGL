namespace Scripts
{
    /// <summary>
    /// Tiny RGB color for [ScriptField] discovery (no UnityEngine dependency).
    /// Host Script Inspector maps this to ImGui ColorEdit3.
    /// </summary>
    public struct Color
    {
        public float R;
        public float G;
        public float B;

        public Color(float r, float g, float b)
        {
            R = r;
            G = g;
            B = b;
        }

        public static Color Clamp01(Color c)
        {
            return new Color(Clamp01(c.R), Clamp01(c.G), Clamp01(c.B));
        }

        static float Clamp01(float v)
        {
            if (v < 0.0f) return 0.0f;
            if (v > 1.0f) return 1.0f;
            return v;
        }
    }
}
