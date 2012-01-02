namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FileTransferException : ApplicationException
    {
        private int replyCode;

        public FileTransferException(string msg) : base(msg)
        {
            this.replyCode = -1;
        }

        public FileTransferException(string msg, Exception innerException) : base(msg, innerException)
        {
            this.replyCode = -1;
        }

        public FileTransferException(string msg, int replyCode) : base(msg)
        {
            this.replyCode = -1;
            this.replyCode = replyCode;
        }

        public FileTransferException(string msg, string replyCode) : base(msg)
        {
            this.replyCode = -1;
            try
            {
                this.replyCode = int.Parse(replyCode);
            }
            catch (FormatException)
            {
                this.replyCode = -1;
            }
        }

        public override string Message
        {
            get
            {
                if (this.replyCode > 0)
                {
                    return string.Concat(new object[] { base.Message, " (code=", this.replyCode, ")" });
                }
                return base.Message;
            }
        }

        public int ReplyCode
        {
            get
            {
                return this.replyCode;
            }
        }
    }
}

