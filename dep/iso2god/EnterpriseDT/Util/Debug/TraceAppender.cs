namespace EnterpriseDT.Util.Debug
{
    using System;
    using System.Diagnostics;

    public class TraceAppender : Appender
    {
        public virtual void Close()
        {
            Trace.Close();
        }

        public virtual void Log(Exception t)
        {
            Trace.WriteLine(t.GetType().FullName + ": " + t.Message);
            Trace.WriteLine(t.StackTrace.ToString());
        }

        public virtual void Log(string msg)
        {
            Trace.WriteLine(msg);
        }
    }
}

