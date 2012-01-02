namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FTPEventArgs : EventArgs
    {
        private int connectionInstance = -1;
        private bool guiThread = false;
        private int taskID = -1;

        public int ConnectionInstanceNumber
        {
            get
            {
                return this.connectionInstance;
            }
            set
            {
                this.connectionInstance = value;
            }
        }

        public bool IsGuiThread
        {
            get
            {
                return this.guiThread;
            }
            set
            {
                this.guiThread = value;
            }
        }

        public int TaskID
        {
            get
            {
                return this.taskID;
            }
            set
            {
                this.taskID = value;
            }
        }
    }
}

