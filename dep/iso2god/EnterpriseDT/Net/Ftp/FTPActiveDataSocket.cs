namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Net;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.IO;
    using System.Net.Sockets;

    public class FTPActiveDataSocket : FTPDataSocket
    {
        internal BaseSocket acceptedSock = null;
        private Logger log;

        internal FTPActiveDataSocket(BaseSocket sock)
        {
            base.sock = sock;
            this.log = Logger.GetLogger("FTPActiveDataSocket");
        }

        internal virtual void AcceptConnection()
        {
            if (this.acceptedSock == null)
            {
                this.acceptedSock = base.sock.Accept(base.timeout);
                base.SetSocketTimeout(this.acceptedSock, base.timeout);
                this.log.Debug("AcceptConnection() succeeded");
            }
        }

        internal override void Close()
        {
            try
            {
                if (this.acceptedSock != null)
                {
                    this.acceptedSock.Close();
                    this.acceptedSock = null;
                }
            }
            finally
            {
                base.sock.Close();
            }
        }

        internal override bool Poll(int microseconds, SelectMode mode)
        {
            if (this.acceptedSock == null)
            {
                throw new IOException("Not accepted yet");
            }
            return this.acceptedSock.Poll(microseconds, mode);
        }

        internal override int Available
        {
            get
            {
                if (this.acceptedSock == null)
                {
                    throw new IOException("Not accepted yet");
                }
                return this.acceptedSock.Available;
            }
        }

        internal override Stream DataStream
        {
            get
            {
                this.AcceptConnection();
                return this.acceptedSock.GetStream();
            }
        }

        internal override int Timeout
        {
            set
            {
                base.timeout = value;
                base.SetSocketTimeout(base.sock, value);
                if (this.acceptedSock != null)
                {
                    base.SetSocketTimeout(this.acceptedSock, value);
                }
            }
        }
    }
}

