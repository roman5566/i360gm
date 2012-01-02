namespace EnterpriseDT.Util
{
    using System;
    using System.Collections;

    internal class StringSplitter
    {
        public static string[] Split(string str)
        {
            ArrayList list = new ArrayList(str.Split(null));
            for (int i = list.Count - 1; i >= 0; i--)
            {
                if (((string) list[i]).Trim().Length == 0)
                {
                    list.RemoveAt(i);
                }
            }
            return (string[]) list.ToArray(typeof(string));
        }
    }
}

