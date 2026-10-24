using System;
using System.Collections.Generic;
using System.Reflection;

namespace Scripts
{
    /// <summary>
    /// Mono reflection bridge for the host Script Inspector.
    /// Refresh(typeName) rediscovers [ScriptField] public static fields on the
    /// loaded type; host builds sliders/toggles/color pickers from Kind + Name
    /// and push/pulls by index — no hand-written GetMoveSpeed / SetPlayerColor.
    /// </summary>
    public static class ScriptFieldMirror
    {
        public const int KindFloat = 0;
        public const int KindInt = 1;
        public const int KindBool = 2;
        public const int KindColor = 3;

        struct Entry
        {
            public FieldInfo Field;
            public int Kind;
            public float Min;
            public float Max;
            public bool HasRange;
        }

        static readonly List<Entry> s_entries = new List<Entry>();

        /// <summary>Rediscover fields on typeName (e.g. "Scripts.ExampleScript").</summary>
        public static void Refresh(string typeName)
        {
            s_entries.Clear();
            if (string.IsNullOrEmpty(typeName))
                return;

            Type t = Type.GetType(typeName);
            if (t == null)
            {
                // Fallback: scan this assembly for Namespace.Name match.
                foreach (Type candidate in Assembly.GetExecutingAssembly().GetTypes())
                {
                    if (candidate.FullName == typeName)
                    {
                        t = candidate;
                        break;
                    }
                }
            }
            if (t == null)
                return;

            FieldInfo[] fields = t.GetFields(
                BindingFlags.Public | BindingFlags.Static | BindingFlags.DeclaredOnly);
            foreach (FieldInfo fi in fields)
            {
                object[] attrs = fi.GetCustomAttributes(typeof(ScriptFieldAttribute), false);
                if (attrs == null || attrs.Length == 0)
                    continue;

                ScriptFieldAttribute attr = (ScriptFieldAttribute)attrs[0];
                int kind;
                Type ft = fi.FieldType;
                if (ft == typeof(float))
                    kind = KindFloat;
                else if (ft == typeof(int))
                    kind = KindInt;
                else if (ft == typeof(bool))
                    kind = KindBool;
                else if (ft == typeof(Color))
                    kind = KindColor;
                else
                    continue; // skip unsupported cleanly

                Entry e = new Entry();
                e.Field = fi;
                e.Kind = kind;
                e.Min = attr.Min;
                e.Max = attr.Max;
                e.HasRange = attr.HasRange;
                s_entries.Add(e);
            }
        }

        /// <summary>
        /// Bind to Scripts.ExampleScript (avoids host needing MonoString for Refresh).
        /// </summary>
        public static void RefreshExample()
        {
            Refresh("Scripts.ExampleScript");
        }

        public static int Count()
        {
            return s_entries.Count;
        }

        public static string GetName(int index)
        {
            if (!Valid(index))
                return "";
            return s_entries[index].Field.Name;
        }

        public static int GetKind(int index)
        {
            if (!Valid(index))
                return -1;
            return s_entries[index].Kind;
        }

        public static float GetMin(int index)
        {
            if (!Valid(index))
                return 0.0f;
            return s_entries[index].Min;
        }

        public static float GetMax(int index)
        {
            if (!Valid(index))
                return 0.0f;
            return s_entries[index].Max;
        }

        public static int HasRange(int index)
        {
            if (!Valid(index))
                return 0;
            return s_entries[index].HasRange ? 1 : 0;
        }

        public static float GetFloat(int index)
        {
            if (!Valid(index) || s_entries[index].Kind != KindFloat)
                return 0.0f;
            return (float)s_entries[index].Field.GetValue(null);
        }

        public static void SetFloat(int index, float v)
        {
            if (!Valid(index) || s_entries[index].Kind != KindFloat)
                return;
            Entry e = s_entries[index];
            if (e.HasRange)
                v = Clamp(v, e.Min, e.Max);
            e.Field.SetValue(null, v);
        }

        public static int GetInt(int index)
        {
            if (!Valid(index) || s_entries[index].Kind != KindInt)
                return 0;
            return (int)s_entries[index].Field.GetValue(null);
        }

        public static void SetInt(int index, int v)
        {
            if (!Valid(index) || s_entries[index].Kind != KindInt)
                return;
            Entry e = s_entries[index];
            if (e.HasRange)
            {
                if (v < (int)e.Min) v = (int)e.Min;
                if (v > (int)e.Max) v = (int)e.Max;
            }
            e.Field.SetValue(null, v);
        }

        public static int GetBool(int index)
        {
            if (!Valid(index) || s_entries[index].Kind != KindBool)
                return 0;
            return ((bool)s_entries[index].Field.GetValue(null)) ? 1 : 0;
        }

        public static void SetBool(int index, int v)
        {
            if (!Valid(index) || s_entries[index].Kind != KindBool)
                return;
            s_entries[index].Field.SetValue(null, v != 0);
        }

        public static float GetColorR(int index)
        {
            if (!Valid(index) || s_entries[index].Kind != KindColor)
                return 0.0f;
            return ((Color)s_entries[index].Field.GetValue(null)).R;
        }

        public static float GetColorG(int index)
        {
            if (!Valid(index) || s_entries[index].Kind != KindColor)
                return 0.0f;
            return ((Color)s_entries[index].Field.GetValue(null)).G;
        }

        public static float GetColorB(int index)
        {
            if (!Valid(index) || s_entries[index].Kind != KindColor)
                return 0.0f;
            return ((Color)s_entries[index].Field.GetValue(null)).B;
        }

        public static void SetColor(int index, float r, float g, float b)
        {
            if (!Valid(index) || s_entries[index].Kind != KindColor)
                return;
            s_entries[index].Field.SetValue(null, Color.Clamp01(new Color(r, g, b)));
        }

        /// <summary>Lookup helper for host render (e.g. PlayerHalf by name).</summary>
        public static int FindIndex(string name)
        {
            if (string.IsNullOrEmpty(name))
                return -1;
            for (int i = 0; i < s_entries.Count; ++i)
            {
                if (s_entries[i].Field.Name == name)
                    return i;
            }
            return -1;
        }

        static bool Valid(int index)
        {
            return index >= 0 && index < s_entries.Count;
        }

        static float Clamp(float v, float lo, float hi)
        {
            if (v < lo) return lo;
            if (v > hi) return hi;
            return v;
        }
    }
}
