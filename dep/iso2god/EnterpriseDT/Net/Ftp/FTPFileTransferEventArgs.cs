namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using System;
    using System.IO;

    public class FTPFileTransferEventArgs : FTPCancelableEventArgs
    {
        private bool append;
        private byte[] byteArray;
        private System.IO.Stream dataStream;
        private long fileSize;
        private DataType localDataType;
        private string localFilePath;
        private string remoteDirectory;
        private string remoteFile;

        internal FTPFileTransferEventArgs(bool canBeCancelled, string remoteFile, string remoteDirectory, long fileSize, bool append, bool cancelled, Exception ex) : base(canBeCancelled, cancelled, ex)
        {
            this.localDataType = DataType.ByteArray;
            this.byteArray = null;
            this.remoteFile = remoteFile;
            this.remoteDirectory = remoteDirectory;
            this.fileSize = fileSize;
            this.append = append;
        }

        internal FTPFileTransferEventArgs(bool canBeCancelled, System.IO.Stream dataStream, string remoteFile, string remoteDirectory, long fileSize, bool append, bool cancelled, Exception ex) : this(canBeCancelled, remoteFile, remoteDirectory, fileSize, append, cancelled, ex)
        {
            this.localDataType = DataType.Stream;
            this.dataStream = dataStream;
        }

        internal FTPFileTransferEventArgs(bool canBeCancelled, byte[] bytes, string remoteFile, string remoteDirectory, long fileSize, bool append, bool cancelled, Exception ex) : this(canBeCancelled, remoteFile, remoteDirectory, fileSize, append, cancelled, ex)
        {
            this.localDataType = DataType.ByteArray;
            this.byteArray = bytes;
        }

        internal FTPFileTransferEventArgs(bool canBeCancelled, string localFilePath, string remoteFile, string remoteDirectory, long fileSize, bool append, bool cancelled, Exception ex) : this(canBeCancelled, remoteFile, remoteDirectory, fileSize, append, cancelled, ex)
        {
            this.localDataType = DataType.File;
            this.localFilePath = localFilePath;
        }

        public bool Appended
        {
            get
            {
                return this.append;
            }
        }

        public byte[] Bytes
        {
            get
            {
                return this.byteArray;
            }
        }

        public override bool Cancel
        {
            get
            {
                return base.Cancel;
            }
            set
            {
                base.Cancel = value;
            }
        }

        public long FileSize
        {
            get
            {
                return this.fileSize;
            }
        }

        public DataType LocalDataType
        {
            get
            {
                return this.localDataType;
            }
        }

        public string LocalDirectory
        {
            get
            {
                return PathUtil.GetFolderPath(this.localFilePath);
            }
        }

        public string LocalFile
        {
            get
            {
                return PathUtil.GetFileName(this.localFilePath);
            }
        }

        public string LocalPath
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

        public string RemoteDirectory
        {
            get
            {
                return PathUtil.GetFolderPath(this.RemotePath);
            }
        }

        public string RemoteFile
        {
            get
            {
                return this.remoteFile;
            }
            set
            {
                this.remoteFile = value;
            }
        }

        public string RemoteFileName
        {
            get
            {
                return PathUtil.GetFileName(this.RemotePath);
            }
        }

        public string RemotePath
        {
            get
            {
                if (!PathUtil.IsAbsolute(this.remoteFile))
                {
                    return PathUtil.Combine(this.remoteDirectory, this.remoteFile);
                }
                return this.remoteFile;
            }
        }

        public System.IO.Stream Stream
        {
            get
            {
                return this.dataStream;
            }
        }

        public enum DataType
        {
            File,
            Stream,
            ByteArray
        }
    }
}

