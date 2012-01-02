namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPTransferCancelledException : FTPException
    {
        public FTPTransferCancelledException(string message) : base(message)
        {
        }
    }
}

