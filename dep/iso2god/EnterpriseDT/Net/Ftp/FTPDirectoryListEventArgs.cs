namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using System;

    public class FTPDirectoryListEventArgs : FTPEventArgs
    {
        private string dirPath;
        private FTPFile[] files;

        internal FTPDirectoryListEventArgs(string dirPath)
        {
            this.dirPath = null;
            this.files = null;
            this.dirPath = dirPath;
        }

        internal FTPDirectoryListEventArgs(string dirPath, FTPFile[] files)
        {
            this.dirPath = null;
            this.files = null;
            this.dirPath = dirPath;
            this.files = files;
        }

        [Obsolete("Use DirectoryName or DirectoryPath")]
        public string Directory
        {
            get
            {
                return this.dirPath;
            }
        }

        public string DirectoryName
        {
            get
            {
                return PathUtil.GetFileName(this.dirPath);
            }
        }

        public string DirectoryPath
        {
            get
            {
                return this.dirPath;
            }
        }

        public FTPFile[] FileInfos
        {
            get
            {
                return this.files;
            }
        }
    }
}

