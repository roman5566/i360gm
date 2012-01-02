namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPCancelableEventArgs : FTPEventArgs
    {
        private bool canBeCancelled;
        private bool cancel;
        private System.Exception ex;

        protected FTPCancelableEventArgs(bool canBeCancelled, bool defaultCancelValue, System.Exception ex)
        {
            this.cancel = defaultCancelValue;
            this.canBeCancelled = canBeCancelled;
            this.ex = ex;
        }

        internal bool CanBeCancelled
        {
            get
            {
                return this.canBeCancelled;
            }
        }

        public virtual bool Cancel
        {
            get
            {
                return this.cancel;
            }
            set
            {
                this.cancel = value;
            }
        }

        public System.Exception Exception
        {
            get
            {
                return this.ex;
            }
        }

        public bool Succeeded
        {
            get
            {
                return (this.ex == null);
            }
        }
    }
}

