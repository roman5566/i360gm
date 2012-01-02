namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPConnectionClosedException : FTPException
    {
        public FTPConnectionClosedException(string message) : base(message)
        {
        }
    }
}

