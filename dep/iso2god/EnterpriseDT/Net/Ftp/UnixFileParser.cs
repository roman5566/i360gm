namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Globalization;
    using System.Text;

    public class UnixFileParser : FTPFileParser
    {
        private const char DIRECTORY_CHAR = 'd';
        private string[] format1 = new string[] { "MMM'-'d'-'yyyy", "MMM'-'dd'-'yyyy" };
        private const string format1a = "MMM'-'d'-'yyyy";
        private const string format1b = "MMM'-'dd'-'yyyy";
        private string[] format2 = new string[] { "MMM'-'d'-'yyyy'-'HH':'mm", "MMM'-'dd'-'yyyy'-'HH':'mm", "MMM'-'d'-'yyyy'-'H':'mm", "MMM'-'dd'-'yyyy'-'H':'mm" };
        private const string format2a = "MMM'-'d'-'yyyy'-'HH':'mm";
        private const string format2b = "MMM'-'dd'-'yyyy'-'HH':'mm";
        private const string format2c = "MMM'-'d'-'yyyy'-'H':'mm";
        private const string format2d = "MMM'-'dd'-'yyyy'-'H':'mm";
        private Logger log = Logger.GetLogger("UnixFileParser");
        private const int MIN_FIELD_COUNT = 8;
        private const char ORDINARY_FILE_CHAR = '-';
        private const string SYMLINK_ARROW = "->";
        private const char SYMLINK_CHAR = 'l';

        private bool IsNumeric(string field)
        {
            for (int i = 0; i < field.Length; i++)
            {
                if (!char.IsDigit(field[i]))
                {
                    return false;
                }
            }
            return true;
        }

        public static bool IsUnix(string raw)
        {
            char ch = raw[0];
            if (((ch != '-') && (ch != 'd')) && (ch != 'l'))
            {
                return false;
            }
            return true;
        }

        public override bool IsValidFormat(string[] listing)
        {
            int num = Math.Min(listing.Length, 10);
            bool flag = false;
            bool flag2 = false;
            for (int i = 0; i < num; i++)
            {
                if (listing[i].Trim().Length != 0)
                {
                    string[] strArray = StringSplitter.Split(listing[i]);
                    if (strArray.Length >= 8)
                    {
                        switch (char.ToLower(strArray[0][0]))
                        {
                            case '-':
                            case 'l':
                            case 'd':
                                flag = true;
                                break;
                        }
                        char ch2 = char.ToLower(strArray[0][1]);
                        if ((ch2 == 'r') || (ch2 == '-'))
                        {
                            flag2 = true;
                        }
                        if (!flag2 && (strArray[0].IndexOf('-', 2) > 0))
                        {
                            flag2 = true;
                        }
                    }
                }
            }
            if (flag && flag2)
            {
                return true;
            }
            this.log.Debug("Not in UNIX format");
            return false;
        }

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
                this.log.Warn(builder.ToString());
                return null;
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
            int num2 = 0;
            if (char.IsDigit(strArray[index][0]))
            {
                string str2 = strArray[index++];
                try
                {
                    num2 = int.Parse(str2);
                }
                catch (FormatException)
                {
                    this.log.Warn("Failed to parse link count: " + str2);
                }
            }
            else if (strArray[index][0] == '-')
            {
                index++;
            }
            string str3 = strArray[index++];
            string str4 = strArray[index++];
            long size = 0L;
            string s = strArray[index];
            if (!char.IsDigit(s[0]) && char.IsDigit(str4[0]))
            {
                s = str4;
                str4 = str3;
                str3 = "";
            }
            else
            {
                index++;
            }
            try
            {
                size = long.Parse(s);
            }
            catch (FormatException)
            {
                this.log.Warn("Failed to parse size: " + s);
            }
            int num4 = -1;
            if (this.IsNumeric(strArray[index]))
            {
                if ((strArray.Length - index) < 5)
                {
                    try
                    {
                        char[] trimChars = new char[] { '0' };
                        strArray[index].TrimStart(trimChars);
                        num4 = int.Parse(strArray[index]);
                        if (num4 > 0x1f)
                        {
                            num4 = -1;
                        }
                    }
                    catch (FormatException)
                    {
                    }
                }
                index++;
            }
            int num5 = index;
            DateTime minValue = DateTime.MinValue;
            StringBuilder builder2 = new StringBuilder(strArray[index++]);
            builder2.Append('-');
            if (num4 > 0)
            {
                builder2.Append(num4);
            }
            else
            {
                builder2.Append(strArray[index++]);
            }
            builder2.Append('-');
            string str6 = strArray[index++];
            if (str6.IndexOf(':') < 0)
            {
                builder2.Append(str6);
                try
                {
                    minValue = DateTime.ParseExact(builder2.ToString(), this.format1, base.ParsingCulture.DateTimeFormat, DateTimeStyles.None);
                    goto Label_0377;
                }
                catch (FormatException)
                {
                    this.log.Warn("Failed to parse date string '" + builder2.ToString() + "'");
                    goto Label_0377;
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
        Label_0377:
            str7 = null;
            string str8 = null;
            int startIndex = 0;
            bool flag3 = true;
            int num8 = (num4 > 0) ? 2 : 3;
            for (int i = num5; i < (num5 + num8); i++)
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
                string str9 = raw.Substring(startIndex).Trim();
                if (!flag2)
                {
                    str7 = str9;
                }
                else
                {
                    startIndex = str9.IndexOf("->");
                    if (startIndex <= 0)
                    {
                        str7 = str9;
                    }
                    else
                    {
                        int length = "->".Length;
                        str7 = str9.Substring(0, startIndex).Trim();
                        if ((startIndex + length) < str9.Length)
                        {
                            str8 = str9.Substring(startIndex + length);
                        }
                    }
                }
            }
            else
            {
                this.log.Warn("Failed to retrieve name: " + raw);
            }
            return new FTPFile(1, raw, str7, size, isDir, ref minValue) { Group = str4, Owner = str3, Link = flag2, LinkCount = num2, LinkedName = str8, Permissions = str };
        }

        public override string ToString()
        {
            return "UNIX";
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

