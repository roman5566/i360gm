namespace EnterpriseDT.Util.Debug
{
    using System;
    using System.IO;

    public class StandardOutputAppender : Appender
    {
        private StreamWriter log = new StreamWriter(Console.OpenStandardOutput());

        public virtual void Close()
        {
            this.log.Flush();
        }

        public virtual void Log(Exception t)
        {
            this.log.WriteLine(t.GetType().FullName + ": " + t.Message);
            this.log.WriteLine(t.StackTrace.ToString());
            this.log.Flush();
        }

        public virtual void Log(string msg)
        {
            this.log.WriteLine(msg);
            this.log.Flush();
        }
    }
}

