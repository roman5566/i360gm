namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Globalization;

    public class OS400FileParser : FTPFileParser
    {
        private static readonly string DATE_FORMAT_1 = "dd'/'MM'/'yy' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_11 = "yy'/'MM'/'dd' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_12 = "yyyy'/'MM'/'dd' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_13 = "yy'.'MM'.'dd' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_2 = "dd'/'MM'/'yyyy' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_21 = "MM'/'dd'/'yy' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_22 = "MM'/'dd'/'yyyy' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_23 = "MM'.'dd'.'yy' 'HH':'mm':'ss";
        private static readonly string DATE_FORMAT_3 = "dd'.'MM'.'yy' 'HH':'mm':'ss";
        private static readonly string DDIR = "*DDIR";
        private static readonly string DIR = "*DIR";
        private int formatIndex = 0;
        private string[][] formats = new string[][] { formats1, formats2, formats3 };
        private static string[] formats1 = new string[] { DATE_FORMAT_1, DATE_FORMAT_2, DATE_FORMAT_3 };
        private static string[] formats2 = new string[] { DATE_FORMAT_11, DATE_FORMAT_12, DATE_FORMAT_13 };
        private static string[] formats3 = new string[] { DATE_FORMAT_21, DATE_FORMAT_22, DATE_FORMAT_23 };
        private Logger log = Logger.GetLogger("OS400FileParser");
        private static readonly string MEM = "*MEM";
        private static readonly int MIN_EXPECTED_FIELD_COUNT = 6;

        private DateTime GetLastModified(string lastModifiedStr)
        {
            DateTime minValue = DateTime.MinValue;
            if (this.formatIndex >= this.formats.Length)
            {
                this.log.Warn("Exhausted formats - failed to parse date");
                return DateTime.MinValue;
            }
            int formatIndex = this.formatIndex;
            int num2 = this.formatIndex;
            while (num2 < this.formats.Length)
            {
                try
                {
                    minValue = DateTime.ParseExact(lastModifiedStr, this.formats[this.formatIndex], base.ParsingCulture.DateTimeFormat, DateTimeStyles.None);
                    if (minValue > DateTime.Now.AddDays(2.0))
                    {
                        this.log.Debug("Swapping to alternate format (found date in future)");
                    }
                    else
                    {
                        break;
                    }
                }
                catch (FormatException)
                {
                }
                num2++;
                this.formatIndex++;
            }
            if (this.formatIndex >= this.formats.Length)
            {
                this.log.Warn("Exhausted formats - failed to parse date");
                return DateTime.MinValue;
            }
            if (this.formatIndex > formatIndex)
            {
                throw new RestartParsingException();
            }
            return minValue;
        }

        public override bool IsValidFormat(string[] listing)
        {
            int num = Math.Min(listing.Length, 10);
            bool flag = false;
            bool flag2 = false;
            bool flag3 = false;
            bool flag4 = false;
            bool flag5 = false;
            bool flag6 = false;
            for (int i = 0; i < num; i++)
            {
                if (listing[i].IndexOf("*DIR") > 0)
                {
                    flag = true;
                }
                else if (listing[i].IndexOf("*FILE") > 0)
                {
                    flag6 = true;
                }
                else if (listing[i].IndexOf("*FLR") > 0)
                {
                    flag5 = true;
                }
                else if (listing[i].IndexOf("*DDIR") > 0)
                {
                    flag2 = true;
                }
                else if (listing[i].IndexOf("*STMF") > 0)
                {
                    flag4 = true;
                }
                else if (listing[i].IndexOf("*LIB") > 0)
                {
                    flag3 = true;
                }
            }
            if (((flag || flag6) || (flag2 || flag3)) || (flag4 || flag5))
            {
                return true;
            }
            this.log.Debug("Not in OS/400 format");
            return false;
        }

        public override FTPFile Parse(string raw)
        {
            string[] strArray = StringSplitter.Split(raw);
            if (strArray.Length <= 0)
            {
                return null;
            }
            if ((strArray.Length >= 2) && strArray[1].Equals(MEM))
            {
                DateTime minValue = DateTime.MinValue;
                string str = strArray[0];
                return new FTPFile(3, raw, strArray[2], 0L, false, ref minValue) { Owner = str };
            }
            if (strArray.Length < MIN_EXPECTED_FIELD_COUNT)
            {
                return null;
            }
            string str3 = strArray[0];
            long size = long.Parse(strArray[1]);
            string lastModifiedStr = strArray[2] + " " + strArray[3];
            DateTime lastModified = this.GetLastModified(lastModifiedStr);
            bool isDir = false;
            if ((strArray[4] == DIR) || (strArray[4] == DDIR))
            {
                isDir = true;
            }
            string name = strArray[5];
            if (name.EndsWith("/"))
            {
                isDir = true;
                name = name.Substring(0, name.Length - 1);
            }
            return new FTPFile(3, raw, name, size, isDir, ref lastModified) { Owner = str3 };
        }

        public override string ToString()
        {
            return "OS400";
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

