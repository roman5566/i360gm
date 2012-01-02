namespace EnterpriseDT.Net
{
    using System;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;

    public abstract class BaseSocket
    {
        protected AddressFamily addressFamily;
        protected ProtocolType protocolType;
        protected SocketType socketType;

        public BaseSocket()
        {
        }

        public BaseSocket(AddressFamily addressFamily, SocketType socketType, ProtocolType protocolType)
        {
            this.addressFamily = addressFamily;
            this.socketType = socketType;
            this.protocolType = protocolType;
        }

        public abstract BaseSocket Accept(int timeout);
        public abstract IAsyncResult BeginAccept(AsyncCallback callback, object state);
        public abstract IAsyncResult BeginReceive(byte[] buffer, int offset, int size, SocketFlags socketFlags, AsyncCallback callback, object state);
        public abstract void Bind(EndPoint localEP);
        public abstract void Close();
        public abstract void Connect(EndPoint remoteEP);
        public abstract BaseSocket EndAccept(IAsyncResult asyncResult);
        public abstract int EndReceive(IAsyncResult asyncResult);
        public abstract Stream GetStream();
        public abstract Stream GetStream(bool ownsSocket);
        public abstract void Listen(int backlog);
        public abstract bool Poll(int microseconds, SelectMode mode);
        public abstract int Receive(byte[] buffer);
        public abstract int Send(byte[] buffer);
        public abstract int Send(byte[] buffer, int offset, int size, SocketFlags socketFlags);
        public abstract void SetSocketOption(SocketOptionLevel optionLevel, SocketOptionName optionName, int optionValue);

        public abstract int Available { get; }

        public abstract bool Connected { get; }

        public abstract EndPoint LocalEndPoint { get; }

        public abstract EndPoint RemoteEndPoint { get; }
    }
}

