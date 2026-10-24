using System;

namespace Scripts
{
    /// <summary>
    /// Unity SerializeField-style marker for the Script Inspector.
    /// Host discovers public static fields with this attribute via reflection
    /// (ScriptFieldMirror) and builds ImGui widgets — no per-field C++ glue.
    /// Supported types: float, int, bool, Color. Unsupported types are skipped.
    /// </summary>
    [AttributeUsage(AttributeTargets.Field, AllowMultiple = false, Inherited = false)]
    public sealed class ScriptFieldAttribute : Attribute
    {
        public float Min { get; }
        public float Max { get; }
        public bool HasRange { get; }

        public ScriptFieldAttribute()
        {
            Min = 0.0f;
            Max = 0.0f;
            HasRange = false;
        }

        public ScriptFieldAttribute(float min, float max)
        {
            Min = min;
            Max = max;
            HasRange = true;
        }
    }
}
