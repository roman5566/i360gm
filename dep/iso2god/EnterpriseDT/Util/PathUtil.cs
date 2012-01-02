namespace EnterpriseDT.Util
{
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Text;

    internal class PathUtil
    {
        private const string SAMEDIR_STRING = ".";
        private const char SEPARATOR_CHAR = '/';
        private const string UPDIR_STRING = "..";

        public static string Combine(string pathLeft, string pathRight)
        {
            if (pathLeft == null)
            {
                pathLeft = "/";
            }
            if (((pathRight == null) || (pathRight == "")) || (pathRight == "."))
            {
                return pathLeft;
            }
            if (pathRight.StartsWith(Separator))
            {
                throw new ArgumentException("Second argument cannot be absolute", "pathRight");
            }
            if (pathLeft.EndsWith(Separator))
            {
                pathLeft = pathLeft.Substring(0, pathLeft.Length - 1);
            }
            return Fix(pathLeft + Separator + pathRight);
        }

        public static string Combine(string path1, string path2, params string[] pathN)
        {
            string pathLeft = Combine(path1, path2);
            foreach (string str2 in pathN)
            {
                pathLeft = Combine(pathLeft, str2);
            }
            return pathLeft;
        }

        public static StringCollection Explode(string path)
        {
            int num2;
            StringCollection strings = new StringCollection();
            int startIndex = 0;
        Label_0008:
            num2 = path.IndexOf(SeparatorChar, startIndex);
            if (num2 >= 0)
            {
                if (startIndex == num2)
                {
                    strings.Add(Separator);
                    startIndex = num2 + 1;
                }
                else
                {
                    strings.Add(path.Substring(startIndex, num2 - startIndex));
                    startIndex = num2;
                }
                goto Label_0008;
            }
            if (startIndex < path.Length)
            {
                strings.Add(path.Substring(startIndex));
            }
            return strings;
        }

        public static StringCollection Fix(StringCollection path)
        {
            if (path.Count == 0)
            {
                return path;
            }
            StringCollection strings = new StringCollection();
            for (int i = 0; i < path.Count; i++)
            {
                string str = path[i];
                if (str == Separator)
                {
                    if ((i == 0) || ((strings.Count > 0) && (strings[strings.Count - 1] != Separator)))
                    {
                        strings.Add(str);
                    }
                }
                else if (str == "..")
                {
                    if (strings.Count == 1)
                    {
                        throw new ArgumentException("Cannot change up a directory from the root");
                    }
                    if ((strings.Count >= 2) && (strings[strings.Count - 2] != ".."))
                    {
                        strings.RemoveAt(strings.Count - 1);
                        strings.RemoveAt(strings.Count - 1);
                    }
                    else
                    {
                        strings.Add(str);
                    }
                }
                else if ((str != "") && (str != "."))
                {
                    strings.Add(str);
                }
            }
            while ((strings.Count > 1) && (strings[strings.Count - 1] == Separator))
            {
                strings.RemoveAt(strings.Count - 1);
            }
            if (strings.Count == 0)
            {
                strings.Add(".");
            }
            return strings;
        }

        public static string Fix(string path)
        {
            return Implode(Fix(Explode(path)));
        }

        public static string GetFileName(string path)
        {
            char ch = '/';
            if (path.IndexOf(ch.ToString()) >= 0)
            {
                return path.Substring(path.LastIndexOf('/') + 1);
            }
            return path;
        }

        public static string GetFolderPath(string path)
        {
            char ch = '/';
            if (path.IndexOf(ch.ToString()) >= 0)
            {
                return path.Substring(0, path.LastIndexOf('/'));
            }
            return path;
        }

        public static string Implode(IEnumerable pathElements)
        {
            return Implode(pathElements, 0, -1);
        }

        public static string Implode(IEnumerable pathElements, int start)
        {
            return Implode(pathElements, start, -1);
        }

        public static string Implode(IEnumerable pathElements, int start, int length)
        {
            StringBuilder builder = new StringBuilder();
            int num = 0;
            foreach (string str in pathElements)
            {
                if ((num >= start) && ((length < 0) || (num < (start + length))))
                {
                    builder.Append(str);
                }
                num++;
            }
            return builder.ToString();
        }

        public static bool IsAbsolute(string path)
        {
            return ((path != null) && path.StartsWith("/"));
        }

        public static string Separator
        {
            get
            {
                char ch = '/';
                return ch.ToString();
            }
        }

        public static char SeparatorChar
        {
            get
            {
                return '/';
            }
        }
    }
}

