namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPMessageEventArgs : FTPEventArgs
    {
        private string message;

        public FTPMessageEventArgs(string message)
        {
            this.message = message;
        }

        public string Message
        {
            get
            {
                return this.message;
            }
        }
    }
}

