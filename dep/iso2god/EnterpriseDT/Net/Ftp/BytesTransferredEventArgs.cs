namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using System;

    public class BytesTransferredEventArgs : FTPEventArgs
    {
        private long byteCount;
        private string remoteFilePath;
        private long resumeOffset;

        public BytesTransferredEventArgs(string remoteFile, long byteCount, long resumeOffset)
        {
            this.remoteFilePath = remoteFile;
            this.byteCount = byteCount;
            this.resumeOffset = resumeOffset;
        }

        public BytesTransferredEventArgs(string remoteDirectory, string remoteFile, long byteCount, long resumeOffset)
        {
            this.byteCount = byteCount;
            this.resumeOffset = resumeOffset;
            this.remoteFilePath = PathUtil.IsAbsolute(remoteFile) ? remoteFile : PathUtil.Combine(remoteDirectory, remoteFile);
        }

        public long ByteCount
        {
            get
            {
                return this.byteCount;
            }
        }

        public string RemoteDirectory
        {
            get
            {
                return PathUtil.GetFolderPath(this.remoteFilePath);
            }
        }

        public string RemoteFile
        {
            get
            {
                return PathUtil.GetFileName(this.remoteFilePath);
            }
        }

        public string RemotePath
        {
            get
            {
                return this.remoteFilePath;
            }
        }

        public long ResumeOffset
        {
            get
            {
                return this.resumeOffset;
            }
        }
    }
}

