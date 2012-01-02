namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPReply
    {
        private string[] data;
        private string replyCode;
        private string replyText;

        public FTPReply(string replyCode, string replyText) : this(replyCode, replyText, null)
        {
        }

        public FTPReply(string replyCode, string replyText, string[] data)
        {
            foreach (char ch in replyCode)
            {
                if (!char.IsDigit(ch))
                {
                    throw new MalformedReplyException("Malformed FTP reply: " + replyCode);
                }
            }
            this.replyCode = replyCode;
            this.replyText = replyText;
            this.data = data;
        }

        public virtual string ReplyCode
        {
            get
            {
                return this.replyCode;
            }
        }

        public virtual string[] ReplyData
        {
            get
            {
                return this.data;
            }
        }

        public virtual string ReplyText
        {
            get
            {
                return this.replyText;
            }
        }
    }
}

