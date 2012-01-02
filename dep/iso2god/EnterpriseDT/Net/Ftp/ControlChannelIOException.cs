namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.IO;

    public class ControlChannelIOException : IOException
    {
        public ControlChannelIOException()
        {
        }

        public ControlChannelIOException(string message) : base(message)
        {
        }
    }
}

