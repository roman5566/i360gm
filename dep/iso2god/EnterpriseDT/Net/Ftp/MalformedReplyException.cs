namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class MalformedReplyException : FTPException
    {
        public MalformedReplyException(string message) : base(message)
        {
        }
    }
}

