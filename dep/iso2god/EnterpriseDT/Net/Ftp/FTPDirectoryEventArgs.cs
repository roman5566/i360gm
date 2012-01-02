namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using System;

    public class FTPDirectoryEventArgs : FTPCancelableEventArgs
    {
        private string newDirectory;
        private string oldDirectory;

        internal FTPDirectoryEventArgs(string oldDirectory, string newDirectory, Exception ex) : base(false, false, ex)
        {
            this.oldDirectory = oldDirectory;
            this.newDirectory = newDirectory;
        }

        internal FTPDirectoryEventArgs(string oldDirectory, string newDirectory, bool cancel, Exception ex) : base(true, cancel, ex)
        {
            this.oldDirectory = oldDirectory;
            this.newDirectory = newDirectory;
        }

        [Obsolete("Use NewDirectoryName or NewDirectoryPath")]
        public string NewDirectory
        {
            get
            {
                return this.newDirectory;
            }
        }

        public string NewDirectoryName
        {
            get
            {
                return PathUtil.GetFileName(this.newDirectory);
            }
        }

        public string NewDirectoryPath
        {
            get
            {
                return this.newDirectory;
            }
        }

        [Obsolete("Use OldDirectoryName or OldDirectoryPath")]
        public string OldDirectory
        {
            get
            {
                return this.oldDirectory;
            }
        }

        public string OldDirectoryName
        {
            get
            {
                return PathUtil.GetFileName(this.oldDirectory);
            }
        }

        public string OldDirectoryPath
        {
            get
            {
                return this.oldDirectory;
            }
        }
    }
}

