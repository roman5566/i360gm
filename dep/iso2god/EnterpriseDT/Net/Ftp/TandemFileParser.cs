namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Globalization;

    public class TandemFileParser : FTPFileParser
    {
        private static readonly string format1 = "d'-'MMM'-'yy HH':'mm':'ss";
        private string[] formats = new string[] { format1 };
        private Logger log = Logger.GetLogger("TandemFileParser");
        private const int MIN_EXPECTED_FIELD_COUNT = 7;
        private char[] trimChars = new char[] { '"' };

        private bool IsHeader(string line)
        {
            return (((line.IndexOf("Code") > 0) && (line.IndexOf("EOF") > 0)) && (line.IndexOf("RWEP") > 0));
        }

        public override bool IsValidFormat(string[] listing)
        {
            return this.IsHeader(listing[0]);
        }

        public override FTPFile Parse(string raw)
        {
            if (this.IsHeader(raw))
            {
                return null;
            }
            string[] strArray = StringSplitter.Split(raw);
            if (strArray.Length < 7)
            {
                return null;
            }
            string name = strArray[0];
            string s = strArray[3] + " " + strArray[4];
            DateTime minValue = DateTime.MinValue;
            try
            {
                minValue = DateTime.ParseExact(s, this.formats, base.ParsingCulture.DateTimeFormat, DateTimeStyles.None);
            }
            catch (FormatException)
            {
                this.log.Warn("Failed to parse date string '" + s + "'");
            }
            bool isDir = false;
            long size = 0L;
            try
            {
                size = long.Parse(strArray[2]);
            }
            catch (FormatException)
            {
                this.log.Warn("Failed to parse size: " + strArray[2]);
            }
            string str3 = strArray[5] + strArray[6];
            string str4 = strArray[7].Trim(this.trimChars);
            return new FTPFile(-1, raw, name, size, isDir, ref minValue) { Owner = str3, Permissions = str4 };
        }

        public override string ToString()
        {
            return "Tandem";
        }

        public override bool TimeIncludesSeconds
        {
            get
            {
                return true;
            }
        }
    }
}

