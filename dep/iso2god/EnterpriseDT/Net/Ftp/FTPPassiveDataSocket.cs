namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Net;
    using System;
    using System.IO;

    public class FTPPassiveDataSocket : FTPDataSocket
    {
        internal FTPPassiveDataSocket(BaseSocket sock)
        {
            base.sock = sock;
        }

        internal override void Close()
        {
            base.sock.Close();
        }

        internal override Stream DataStream
        {
            get
            {
                return base.sock.GetStream();
            }
        }

        internal override int Timeout
        {
            set
            {
                base.SetSocketTimeout(base.sock, value);
            }
        }
    }
}

