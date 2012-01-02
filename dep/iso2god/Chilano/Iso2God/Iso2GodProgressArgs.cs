namespace Chilano.Iso2God
{
    using System;
    using System.ComponentModel;

    public class Iso2GodProgressArgs : EventArgs
    {
        public string Message;
        public int Percentage;

        public Iso2GodProgressArgs(ProgressChangedEventArgs e)
        {
            this.Percentage = e.ProgressPercentage;
            this.Message = e.UserState.ToString();
        }
    }
}

