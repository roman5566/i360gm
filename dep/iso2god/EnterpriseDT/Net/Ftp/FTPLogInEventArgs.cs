namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPLogInEventArgs : FTPEventArgs
    {
        private bool hasLoggedIn;
        private string password;
        private string userName;

        internal FTPLogInEventArgs(string userName, string password, bool hasLoggedIn)
        {
            this.userName = userName;
            this.password = password;
            this.hasLoggedIn = hasLoggedIn;
        }

        public bool HasLoggedIn
        {
            get
            {
                return this.hasLoggedIn;
            }
        }

        public string Password
        {
            get
            {
                return this.password;
            }
        }

        public string UserName
        {
            get
            {
                return this.userName;
            }
        }
    }
}

