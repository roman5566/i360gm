namespace EnterpriseDT.Util.Debug
{
    using System;
    using System.IO;
    using System.Text;

    public class RollingFileAppender : FileAppender
    {
        private const int CHECK_COUNT_FREQUENCY = 100;
        private const long DEFAULT_MAXSIZE = 0xa00000L;
        private long maxFileSize;
        private int maxSizeRollBackups;
        private int sizeCheckCount;

        public RollingFileAppender(string fileName) : base(fileName)
        {
            this.maxFileSize = 0xa00000L;
            this.sizeCheckCount = 0;
            this.maxSizeRollBackups = 1;
        }

        public RollingFileAppender(string fileName, long maxFileSize) : base(fileName)
        {
            this.maxFileSize = 0xa00000L;
            this.sizeCheckCount = 0;
            this.maxSizeRollBackups = 1;
            this.maxFileSize = maxFileSize;
        }

        private void CheckForRollover()
        {
            try
            {
                long position = base.fileStream.Position;
                if (this.sizeCheckCount >= 100)
                {
                    position = base.fileStream.Length;
                    this.sizeCheckCount = 0;
                }
                else
                {
                    this.sizeCheckCount++;
                }
                if (position > this.maxFileSize)
                {
                    this.Rollover();
                }
            }
            catch (Exception exception)
            {
                Console.WriteLine("Failed to rollover log files (" + exception.Message + ")");
            }
        }

        public override void Log(Exception t)
        {
            StringBuilder builder = new StringBuilder(t.GetType().FullName);
            builder.Append(": ").Append(t.Message);
            if (!base.closed)
            {
                this.CheckForRollover();
                base.logger.WriteLine(builder.ToString());
                base.logger.WriteLine(t.StackTrace.ToString());
                base.logger.Flush();
            }
            else
            {
                Console.WriteLine(builder.ToString());
            }
        }

        public override void Log(string msg)
        {
            if (!base.closed)
            {
                this.CheckForRollover();
                base.logger.WriteLine(msg);
                base.logger.Flush();
            }
            else
            {
                Console.WriteLine(msg);
            }
        }

        private void Rollover()
        {
            this.Close();
            FileInfo info = new FileInfo(base.FileName);
            if (this.maxSizeRollBackups == 0)
            {
                info.Delete();
            }
            else
            {
                FileInfo info2 = new FileInfo(base.FileName + "." + this.maxSizeRollBackups);
                if (info2.Exists)
                {
                    info2.Delete();
                }
                for (int i = this.maxSizeRollBackups - 1; i > 0; i--)
                {
                    info2 = new FileInfo(base.FileName + "." + i);
                    if (info2.Exists)
                    {
                        info2.MoveTo(base.FileName + "." + (i + 1));
                    }
                }
                info.MoveTo(base.FileName + ".1");
            }
            this.sizeCheckCount = 0;
            base.Open();
        }

        public long MaxFileSize
        {
            get
            {
                return this.maxFileSize;
            }
            set
            {
                this.maxFileSize = value;
            }
        }

        public int MaxSizeRollBackups
        {
            get
            {
                return this.maxSizeRollBackups;
            }
            set
            {
                this.maxSizeRollBackups = (value >= 0) ? value : 0;
            }
        }
    }
}

