namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.ComponentModel;
    using System.Text;

    public class VMSFileParser : FTPFileParser
    {
        private int blocksize = 0x80000;
        private const int DEFAULT_BLOCKSIZE = 0x80000;
        private static readonly string DIR = ".DIR";
        private static readonly string HDR = "Directory";
        private Logger log = Logger.GetLogger("VMSFileParser");
        private static readonly int MIN_EXPECTED_FIELD_COUNT = 4;
        private static readonly string TOTAL = "Total";
        private bool versionInName = false;

        public override bool IsMultiLine()
        {
            return true;
        }

        public override bool IsValidFormat(string[] listing)
        {
            int num = Math.Min(listing.Length, 10);
            bool flag = false;
            bool flag2 = false;
            bool flag3 = false;
            bool flag4 = false;
            for (int i = 0; i < num; i++)
            {
                if (listing[i].Trim().Length != 0)
                {
                    int num3 = 0;
                    if ((((num3 = listing[i].IndexOf(';')) > 0) && (++num3 < listing[i].Length)) && char.IsDigit(listing[i][num3]))
                    {
                        flag = true;
                    }
                    if (listing[i].IndexOf(',') > 0)
                    {
                        flag2 = true;
                    }
                    if (listing[i].IndexOf('[') > 0)
                    {
                        flag3 = true;
                    }
                    if (listing[i].IndexOf(']') > 0)
                    {
                        flag4 = true;
                    }
                }
            }
            if ((flag && flag2) && (flag3 && flag4))
            {
                return true;
            }
            this.log.Debug("Not in VMS format");
            return false;
        }

        public override FTPFile Parse(string raw)
        {
            string[] fields = StringSplitter.Split(raw);
            if (fields.Length <= 0)
            {
                return null;
            }
            if ((fields.Length >= 2) && fields[0].Equals(HDR))
            {
                return null;
            }
            if ((fields.Length > 0) && fields[0].Equals(TOTAL))
            {
                return null;
            }
            if (fields.Length < MIN_EXPECTED_FIELD_COUNT)
            {
                return null;
            }
            string name = fields[0];
            int length = name.LastIndexOf(';');
            if (length <= 0)
            {
                this.log.Warn("File version number not found in name '" + name + "'");
                return null;
            }
            string str2 = name.Substring(0, length);
            string s = fields[0].Substring(length + 1);
            try
            {
                long.Parse(s);
            }
            catch (FormatException)
            {
            }
            bool isDir = false;
            if (str2.EndsWith(DIR))
            {
                isDir = true;
                name = str2.Substring(0, str2.Length - DIR.Length);
            }
            if (!this.versionInName && !isDir)
            {
                name = str2;
            }
            int index = fields[1].IndexOf('/');
            string str4 = fields[1];
            if (index > 0)
            {
                str4 = fields[1].Substring(0, index);
            }
            long size = long.Parse(str4) * this.blocksize;
            string str5 = this.TweakDateString(fields);
            DateTime minValue = DateTime.MinValue;
            try
            {
                minValue = DateTime.Parse(str5.ToString(), base.ParsingCulture.DateTimeFormat);
            }
            catch (FormatException)
            {
                this.log.Warn("Failed to parse date string '" + str5 + "'");
            }
            string str6 = null;
            string str7 = null;
            if (((fields.Length >= 5) && (fields[4][0] == '[')) && (fields[4][fields[4].Length - 1] == ']'))
            {
                int num4 = fields[4].IndexOf(',');
                if (num4 < 0)
                {
                    str7 = fields[4].Substring(1, fields[4].Length - 2);
                    str6 = "";
                }
                else
                {
                    str6 = fields[4].Substring(1, num4 - 1);
                    str7 = fields[4].Substring(num4 + 1, (fields[4].Length - num4) - 2);
                }
            }
            string str8 = null;
            if (((fields.Length >= 6) && (fields[5][0] == '(')) && (fields[5][fields[5].Length - 1] == ')'))
            {
                str8 = fields[5].Substring(1, fields[5].Length - 2);
            }
            return new FTPFile(2, raw, name, size, isDir, ref minValue) { Group = str6, Owner = str7, Permissions = str8 };
        }

        public override string ToString()
        {
            return "VMS";
        }

        private string TweakDateString(string[] fields)
        {
            StringBuilder builder = new StringBuilder();
            bool flag = false;
            for (int i = 0; i < fields[2].Length; i++)
            {
                if (!char.IsLetter(fields[2][i]))
                {
                    builder.Append(fields[2][i]);
                }
                else if (!flag)
                {
                    builder.Append(fields[2][i]);
                    flag = true;
                }
                else
                {
                    builder.Append(char.ToLower(fields[2][i]));
                }
            }
            builder.Append(" ").Append(fields[3]);
            return builder.ToString();
        }

        [DefaultValue(0x80000)]
        public int BlockSize
        {
            get
            {
                return this.blocksize;
            }
            set
            {
                this.blocksize = value;
            }
        }

        public override bool TimeIncludesSeconds
        {
            get
            {
                return false;
            }
        }

        [DefaultValue(false)]
        public bool VersionInName
        {
            get
            {
                return this.versionInName;
            }
            set
            {
                this.versionInName = value;
            }
        }
    }
}

