namespace EnterpriseDT.Util.Debug
{
    using System;
    using System.IO;

    public class FileAppender : Appender
    {
        protected bool closed = false;
        private string fileName;
        protected FileStream fileStream;
        protected TextWriter logger;

        public FileAppender(string fileName)
        {
            this.fileName = fileName;
            this.Open();
        }

        public virtual void Close()
        {
            this.logger.Flush();
            this.logger.Close();
            this.logger = null;
            this.fileStream = null;
            this.closed = true;
        }

        public virtual void Log(Exception t)
        {
            if (!this.closed)
            {
                this.logger.WriteLine(t.GetType().FullName + ": " + t.Message);
                this.logger.WriteLine(t.StackTrace.ToString());
                this.logger.Flush();
            }
            else
            {
                Console.WriteLine(t.GetType().FullName + ": " + t.Message);
            }
        }

        public virtual void Log(string msg)
        {
            if (!this.closed)
            {
                this.logger.WriteLine(msg);
                this.logger.Flush();
            }
            else
            {
                Console.WriteLine(msg);
            }
        }

        protected void Open()
        {
            this.fileStream = new FileStream(this.fileName, FileMode.Append, FileAccess.Write, FileShare.ReadWrite);
            this.logger = TextWriter.Synchronized(new StreamWriter(this.fileStream));
            this.closed = false;
        }

        public string FileName
        {
            get
            {
                return this.fileName;
            }
        }
    }
}

