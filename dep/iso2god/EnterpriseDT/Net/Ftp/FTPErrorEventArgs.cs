namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.Windows.Forms;

    public class FTPErrorEventArgs : FTPEventArgs
    {
        private System.Exception exception;
        private object[] methodArguments;
        private string methodName;

        internal FTPErrorEventArgs(System.Exception exception)
        {
            this.exception = exception;
        }

        internal FTPErrorEventArgs(System.Exception exception, string methodName, object[] methodArguments)
        {
            this.exception = exception;
            this.methodName = methodName;
            this.methodArguments = methodArguments;
        }

        public void ShowMessageBox()
        {
            this.ShowMessageBox(null, false);
        }

        public void ShowMessageBox(bool showDetail)
        {
            this.ShowMessageBox(null, showDetail);
        }

        public void ShowMessageBox(IWin32Window owner)
        {
            this.ShowMessageBox(owner, false);
        }

        public void ShowMessageBox(IWin32Window owner, bool showDetail)
        {
            string message;
            System.Exception innerException = this.exception;
            while (innerException.InnerException != null)
            {
                innerException = innerException.InnerException;
            }
            if (showDetail)
            {
                message = string.Format("{0}: {1}\n{2}", innerException.GetType(), innerException.Message, innerException.StackTrace);
            }
            else
            {
                message = innerException.Message;
            }
            MessageBox.Show(owner, message, "FTP Error", MessageBoxButtons.OK, MessageBoxIcon.Hand);
        }

        public object[] Arguments
        {
            get
            {
                return this.methodArguments;
            }
        }

        public System.Exception Exception
        {
            get
            {
                return this.exception;
            }
        }

        public string SyncMethodName
        {
            get
            {
                return this.methodName;
            }
        }
    }
}

