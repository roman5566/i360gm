namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPConnectionEventArgs : FTPEventArgs
    {
        private bool connected;
        private string serverAddress;
        private int serverPort;

        internal FTPConnectionEventArgs(string serverAddress, int serverPort, bool connected)
        {
            this.serverAddress = serverAddress;
            this.serverPort = serverPort;
            this.connected = connected;
        }

        public bool IsConnected
        {
            get
            {
                return this.connected;
            }
        }

        public string ServerAddress
        {
            get
            {
                return this.serverAddress;
            }
        }

        public int ServerPort
        {
            get
            {
                return this.serverPort;
            }
        }
    }
}

