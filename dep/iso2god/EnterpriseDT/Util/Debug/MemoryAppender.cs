namespace EnterpriseDT.Util.Debug
{
    using System;
    using System.Collections;
    using System.IO;

    public class MemoryAppender : Appender
    {
        public const int DEFAULT_MAX_MESSAGES = 0x3e8;
        private int maxMessages;
        private ArrayList messages;

        public MemoryAppender()
        {
            this.maxMessages = 0x3e8;
            this.messages = new ArrayList();
        }

        public MemoryAppender(int maxMessages)
        {
            this.maxMessages = 0x3e8;
            this.messages = new ArrayList();
            this.maxMessages = maxMessages;
        }

        private void AddMessage(string msg)
        {
            lock (this.messages.SyncRoot)
            {
                if (this.messages.Count == this.maxMessages)
                {
                    this.messages.RemoveAt(0);
                }
                this.messages.Add(msg);
            }
        }

        public virtual void Close()
        {
        }

        public virtual void Log(Exception t)
        {
            this.AddMessage(t.GetType().FullName + ": " + t.Message);
            this.AddMessage(t.StackTrace.ToString());
        }

        public virtual void Log(string msg)
        {
            this.AddMessage(msg);
        }

        public void Write(StreamWriter stream)
        {
            foreach (string str in this.Messages)
            {
                stream.WriteLine(str);
            }
        }

        public void Write(string path)
        {
            using (StreamWriter writer = File.CreateText(path))
            {
                this.Write(writer);
            }
        }

        public int MaxMessages
        {
            get
            {
                return this.maxMessages;
            }
            set
            {
                lock (this.messages.SyncRoot)
                {
                    this.maxMessages = value;
                }
            }
        }

        public string[] Messages
        {
            get
            {
                return (string[]) this.messages.ToArray(typeof(string));
            }
        }
    }
}

