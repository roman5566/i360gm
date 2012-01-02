namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Globalization;

    public class WindowsFileParser : FTPFileParser
    {
        private const string DIR = "<DIR>";
        private static readonly string format1 = "MM'-'dd'-'yy hh':'mmtt";
        private static readonly string format2 = "MM'-'dd'-'yy HH':'mm";
        private string[] formats = new string[] { format1, format2 };
        private Logger log = Logger.GetLogger("WindowsFileParser");
        private const int MIN_EXPECTED_FIELD_COUNT = 4;
        private char[] sep = new char[] { ' ' };

        public override bool IsValidFormat(string[] listing)
        {
            int num = Math.Min(listing.Length, 10);
            bool flag = false;
            bool flag2 = false;
            bool flag3 = false;
            for (int i = 0; i < num; i++)
            {
                if (listing[i].Trim().Length != 0)
                {
                    string[] strArray = StringSplitter.Split(listing[i]);
                    if (strArray.Length >= 4)
                    {
                        if (char.IsDigit(strArray[0][0]) && char.IsDigit(strArray[0][strArray[0].Length - 1]))
                        {
                            flag = true;
                        }
                        if (strArray[1].IndexOf(':') > 0)
                        {
                            flag2 = true;
                        }
                        if ((strArray[2].ToUpper() == "<DIR>") || char.IsDigit(strArray[2][0]))
                        {
                            flag3 = true;
                        }
                    }
                }
            }
            if ((flag && flag2) && flag3)
            {
                return true;
            }
            this.log.Debug("Not in Windows format");
            return false;
        }

        public override FTPFile Parse(string raw)
        {
            string[] strArray = StringSplitter.Split(raw);
            if (strArray.Length < 4)
            {
                return null;
            }
            string s = strArray[0] + " " + strArray[1];
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
            if (strArray[2].ToUpper().Equals("<DIR>".ToUpper()))
            {
                isDir = true;
            }
            else
            {
                try
                {
                    size = long.Parse(strArray[2]);
                }
                catch (FormatException)
                {
                    this.log.Warn("Failed to parse size: " + strArray[2]);
                }
            }
            int startIndex = 0;
            bool flag2 = true;
            for (int i = 0; i < 3; i++)
            {
                startIndex = raw.IndexOf(strArray[i], startIndex);
                if (startIndex < 0)
                {
                    flag2 = false;
                    break;
                }
                startIndex += strArray[i].Length;
            }
            string name = null;
            if (flag2)
            {
                name = raw.Substring(startIndex).Trim();
            }
            else
            {
                this.log.Warn("Failed to retrieve name: " + raw);
            }
            return new FTPFile(0, raw, name, size, isDir, ref minValue);
        }

        public override string ToString()
        {
            return "Windows";
        }

        public override bool TimeIncludesSeconds
        {
            get
            {
                return false;
            }
        }
    }
}

