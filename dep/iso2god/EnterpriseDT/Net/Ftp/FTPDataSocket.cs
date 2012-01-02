namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Net;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;

    public abstract class FTPDataSocket
    {
        private Logger log = Logger.GetLogger("FTPDataSocket");
        internal BaseSocket sock = null;
        internal int timeout = 0;

        protected FTPDataSocket()
        {
        }

        internal abstract void Close();
        internal virtual bool Poll(int microseconds, SelectMode mode)
        {
            return this.sock.Poll(microseconds, mode);
        }

        internal void SetSocketTimeout(BaseSocket sock, int timeout)
        {
            if (timeout > 0)
            {
                try
                {
                    sock.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout, timeout);
                    sock.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout, timeout);
                }
                catch (SocketException exception)
                {
                    this.log.Warn("Failed to set socket timeout: " + exception.Message);
                }
            }
        }

        internal virtual int Available
        {
            get
            {
                return this.sock.Available;
            }
        }

        internal abstract Stream DataStream { get; }

        internal int LocalPort
        {
            get
            {
                return ((IPEndPoint) this.sock.LocalEndPoint).Port;
            }
        }

        internal virtual int Timeout
        {
            get
            {
                return this.timeout;
            }
            set
            {
                this.timeout = value;
                this.SetSocketTimeout(this.sock, value);
            }
        }
    }
}

