namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Globalization;
    using System.Text;

    public class UnixFileParser2 : FTPFileParser
    {
        private const char DIRECTORY_CHAR = 'd';
        private string[] format1 = new string[] { "MMM'-'d'-'yyyy", "MMM'-'dd'-'yyyy" };
        private const string format1a = "MMM'-'d'-'yyyy";
        private const string format1b = "MMM'-'dd'-'yyyy";
        private string[] format2 = new string[] { "MMM'-'d'-'yyyy'-'HH':'mm:ss", "MMM'-'dd'-'yyyy'-'HH':'mm:ss", "MMM'-'d'-'yyyy'-'H':'mm:ss", "MMM'-'dd'-'yyyy'-'H':'mm:ss" };
        private const string format2a = "MMM'-'d'-'yyyy'-'HH':'mm:ss";
        private const string format2b = "MMM'-'dd'-'yyyy'-'HH':'mm:ss";
        private const string format2c = "MMM'-'d'-'yyyy'-'H':'mm:ss";
        private const string format2d = "MMM'-'dd'-'yyyy'-'H':'mm:ss";
        private Logger log = Logger.GetLogger("UnixFileParser2");
        private const int MIN_FIELD_COUNT = 8;
        private const char ORDINARY_FILE_CHAR = '-';
        private const string SYMLINK_ARROW = "->";
        private const char SYMLINK_CHAR = 'l';

        public override FTPFile Parse(string raw)
        {
            string str7;
            char ch = raw[0];
            if (((ch != '-') && (ch != 'd')) && (ch != 'l'))
            {
                return null;
            }
            string[] strArray = StringSplitter.Split(raw);
            if (strArray.Length < 8)
            {
                StringBuilder builder = new StringBuilder("Unexpected number of fields in listing '");
                builder.Append(raw).Append("' - expected minimum ").Append(8).Append(" fields but found ").Append(strArray.Length).Append(" fields");
                throw new FormatException(builder.ToString());
            }
            int index = 0;
            string str = strArray[index++];
            ch = str[0];
            bool isDir = false;
            bool flag2 = false;
            switch (ch)
            {
                case 'd':
                    isDir = true;
                    break;

                case 'l':
                    flag2 = true;
                    break;
            }
            string str2 = strArray[index++];
            int num2 = 0;
            if (char.IsDigit(strArray[index][0]))
            {
                string str3 = strArray[index++];
                try
                {
                    num2 = int.Parse(str3);
                }
                catch (FormatException)
                {
                    this.log.Warn("Failed to parse link count: " + str3);
                }
            }
            string str4 = strArray[index++];
            long size = 0L;
            string s = strArray[index++];
            try
            {
                size = long.Parse(s);
            }
            catch (FormatException)
            {
                this.log.Warn("Failed to parse size: " + s);
            }
            int num4 = index;
            DateTime minValue = DateTime.MinValue;
            StringBuilder builder2 = new StringBuilder(strArray[index++]);
            builder2.Append('-').Append(strArray[index++]).Append('-');
            string str6 = strArray[index++];
            if (str6.IndexOf(':') < 0)
            {
                builder2.Append(str6);
                try
                {
                    minValue = DateTime.ParseExact(builder2.ToString(), this.format1, base.ParsingCulture.DateTimeFormat, DateTimeStyles.None);
                    goto Label_0295;
                }
                catch (FormatException)
                {
                    this.log.Warn("Failed to parse date string '" + builder2.ToString() + "'");
                    goto Label_0295;
                }
            }
            int year = base.ParsingCulture.Calendar.GetYear(DateTime.Now);
            builder2.Append(year).Append('-').Append(str6);
            try
            {
                minValue = DateTime.ParseExact(builder2.ToString(), this.format2, base.ParsingCulture.DateTimeFormat, DateTimeStyles.None);
            }
            catch (FormatException)
            {
                this.log.Warn("Failed to parse date string '" + builder2.ToString() + "'");
            }
            if (minValue > DateTime.Now.AddDays(2.0))
            {
                minValue = minValue.AddYears(-1);
            }
        Label_0295:
            str7 = null;
            int startIndex = 0;
            bool flag3 = true;
            for (int i = num4; i < (num4 + 3); i++)
            {
                startIndex = raw.IndexOf(strArray[i], startIndex);
                if (startIndex < 0)
                {
                    flag3 = false;
                    break;
                }
                startIndex += strArray[i].Length;
            }
            if (flag3)
            {
                str7 = raw.Substring(startIndex).Trim();
            }
            else
            {
                this.log.Warn("Failed to retrieve name: " + raw);
            }
            return new FTPFile(1, raw, str7, size, isDir, ref minValue) { Group = str2, Owner = str4, Link = flag2, LinkCount = num2, Permissions = str };
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

