namespace Chilano.Iso2God
{
    using System;
    using System.ComponentModel;

    public class Iso2GodCompletedArgs : EventArgs
    {
        public bool Cancelled;
        public string ContainerId;
        public Exception Error;
        public string Message;

        public Iso2GodCompletedArgs(RunWorkerCompletedEventArgs e)
        {
            this.Message = (e.Result == null) ? "Error!" : e.Result.ToString();
            this.Cancelled = e.Cancelled;
            this.Error = e.Error;
        }

        public Iso2GodCompletedArgs(RunWorkerCompletedEventArgs e, string ContainerId)
        {
            this.Message = (e.Result == null) ? "Error!" : e.Result.ToString();
            this.Cancelled = e.Cancelled;
            this.Error = e.Error;
            this.ContainerId = ContainerId;
        }
    }
}

