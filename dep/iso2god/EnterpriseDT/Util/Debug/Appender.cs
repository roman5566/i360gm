namespace EnterpriseDT.Util.Debug
{
    using System;

    public interface Appender
    {
        void Close();
        void Log(Exception t);
        void Log(string msg);
    }
}

