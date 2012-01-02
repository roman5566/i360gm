namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using System;
    using System.ComponentModel;

    public class FTPFileRenameEventArgs : FTPCancelableEventArgs
    {
        private string newFilePath;
        private string oldFilePath;

        internal FTPFileRenameEventArgs(bool canBeCancelled, string oldFilePath, string newFilePath, bool cancel, Exception ex) : base(canBeCancelled, cancel, ex)
        {
            this.oldFilePath = oldFilePath;
            this.newFilePath = newFilePath;
        }

        public string NewDirectory
        {
            get
            {
                return PathUtil.GetFolderPath(this.newFilePath);
            }
        }

        public string NewFileName
        {
            get
            {
                return PathUtil.GetFileName(this.newFilePath);
            }
        }

        public string NewFilePath
        {
            get
            {
                return this.newFilePath;
            }
        }

        public string OldDirectory
        {
            get
            {
                return PathUtil.GetFolderPath(this.oldFilePath);
            }
        }

        public string OldFileName
        {
            get
            {
                return PathUtil.GetFileName(this.oldFilePath);
            }
        }

        public string OldFilePath
        {
            get
            {
                return this.oldFilePath;
            }
        }

        [EditorBrowsable(EditorBrowsableState.Never), Obsolete("Use Cancel")]
        public bool RenameCompleted
        {
            get
            {
                return !base.Cancel;
            }
        }
    }
}

