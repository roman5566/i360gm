namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.Globalization;

    public abstract class FTPFileParser
    {
        private CultureInfo parserCulture = CultureInfo.InvariantCulture;

        protected FTPFileParser()
        {
        }

        public virtual bool IsMultiLine()
        {
            return false;
        }

        public virtual bool IsValidFormat(string[] listing)
        {
            return false;
        }

        public abstract FTPFile Parse(string raw);

        public CultureInfo ParsingCulture
        {
            get
            {
                return this.parserCulture;
            }
            set
            {
                this.parserCulture = value;
            }
        }

        public abstract bool TimeIncludesSeconds { get; }
    }
}

