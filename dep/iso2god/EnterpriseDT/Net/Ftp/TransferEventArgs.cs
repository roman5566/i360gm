namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.IO;

    public class TransferEventArgs : FTPEventArgs
    {
        private TransferDirection direction;
        private byte[] localByteArray;
        private string localFilePath;
        private Stream localStream;
        private string remoteFilename;
        private FTPTransferType transferType;

        public TransferEventArgs(Stream localStream, string remoteFilename, TransferDirection direction, FTPTransferType transferType)
        {
            this.localStream = localStream;
            this.remoteFilename = remoteFilename;
            this.direction = direction;
            this.transferType = transferType;
        }

        public TransferEventArgs(byte[] localByteArray, string remoteFilename, TransferDirection direction, FTPTransferType transferType)
        {
            this.localByteArray = localByteArray;
            this.remoteFilename = remoteFilename;
            this.direction = direction;
            this.transferType = transferType;
        }

        public TransferEventArgs(string localFilePath, string remoteFilename, TransferDirection direction, FTPTransferType transferType)
        {
            this.localFilePath = localFilePath;
            this.remoteFilename = remoteFilename;
            this.direction = direction;
            this.transferType = transferType;
        }

        public TransferDirection Direction
        {
            get
            {
                return this.direction;
            }
        }

        public byte[] LocalByteArray
        {
            get
            {
                return this.localByteArray;
            }
        }

        public string LocalFilePath
        {
            get
            {
                return this.localFilePath;
            }
            set
            {
                this.localFilePath = value;
            }
        }

        public Stream LocalStream
        {
            get
            {
                return this.localStream;
            }
        }

        public string RemoteFilename
        {
            get
            {
                return this.remoteFilename;
            }
        }

        public FTPTransferType TransferType
        {
            get
            {
                return this.transferType;
            }
        }
    }
}

