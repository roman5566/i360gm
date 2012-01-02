namespace EnterpriseDT.Net
{
    using System;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;

    public class StandardSocket : BaseSocket
    {
        private System.Net.Sockets.Socket socket;

        protected StandardSocket(System.Net.Sockets.Socket socket)
        {
            this.socket = socket;
        }

        public StandardSocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType) : base(addressFamily, socketType, protocolType)
        {
            this.socket = new System.Net.Sockets.Socket(addressFamily, socketType, protocolType);
        }

        public override BaseSocket Accept(int timeout)
        {
            if (!this.socket.Poll(timeout * 0x3e8, SelectMode.SelectRead))
            {
                this.socket.Close();
                throw new IOException("Failed to accept connection within timeout period (" + timeout + ")");
            }
            return new StandardSocket(this.socket.Accept());
        }

        public override IAsyncResult BeginAccept(AsyncCallback callback, object state)
        {
            return this.socket.BeginAccept(callback, state);
        }

        public override IAsyncResult BeginReceive(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, object state)
        {
            return this.socket.BeginReceive(buffer, offset, size, socketFlags, callback, state);
        }

        public override void Bind(EndPoint localEP)
        {
            this.socket.Bind(localEP);
        }

        public override void Close()
        {
            this.socket.Close();
            this.socket = null;
        }

        public override void Connect(EndPoint remoteEP)
        {
            if (this.socket == null)
            {
                this.socket = new System.Net.Sockets.Socket(base.addressFamily, base.socketType, base.protocolType);
            }
            this.socket.Connect(remoteEP);
        }

        public override BaseSocket EndAccept(IAsyncResult asyncResult)
        {
            return new StandardSocket(this.socket.EndAccept(asyncResult));
        }

        public override int EndReceive(IAsyncResult asyncResult)
        {
            return this.socket.EndReceive(asyncResult);
        }

        public override Stream GetStream()
        {
            return new NetworkStream(this.socket, true);
        }

        public override Stream GetStream(bool ownsSocket)
        {
            return new NetworkStream(this.socket, ownsSocket);
        }

        public override void Listen(int backlog)
        {
            this.socket.Listen(backlog);
        }

        public override bool Poll(int microseconds, SelectMode mode)
        {
            return this.socket.Poll(microseconds, mode);
        }

        public override int Receive(byte[] buffer)
        {
            return this.socket.Receive(buffer);
        }

        public override int Send(byte[] buffer)
        {
            return this.socket.Send(buffer);
        }

        public override int Send(byte[] buffer, int offset, int size, SocketFlags socketFlags)
        {
            return this.socket.Send(buffer, offset, size, socketFlags);
        }

        public override void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionValue)
        {
            this.socket.SetSocketOption(optionLevel, optionName, optionValue);
        }

        public override int Available
        {
            get
            {
                return this.socket.Available;
            }
        }

        public override bool Connected
        {
            get
            {
                return ((this.socket != null) && this.socket.Connected);
            }
        }

        public override EndPoint LocalEndPoint
        {
            get
            {
                return this.socket.LocalEndPoint;
            }
        }

        public override EndPoint RemoteEndPoint
        {
            get
            {
                return this.socket.RemoteEndPoint;
            }
        }

        public System.Net.Sockets.Socket Socket
        {
            get
            {
                return this.socket;
            }
        }
    }
}

