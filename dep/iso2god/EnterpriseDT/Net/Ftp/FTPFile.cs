namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.Globalization;
    using System.Text;

    public class FTPFile
    {
        protected internal string fileName;
        protected internal string fileOwner;
        protected string filePath;
        protected internal string filePermissions;
        protected internal long fileSize;
        private static readonly string format = "dd-MM-yyyy HH:mm";
        protected internal bool isDir;
        protected internal bool isLink;
        protected FTPFile[] kids;
        protected internal DateTime lastModifiedTime;
        protected internal string linkedFileName;
        protected internal int linkNum;
        public const int OS400 = 3;
        protected internal string rawRep;
        private int type;
        public const int UNIX = 1;
        public const int UNKNOWN = -1;
        protected internal string userGroup;
        public const int VMS = 2;
        public const int WINDOWS = 0;

        internal FTPFile(string name, long size, bool isDir, DateTime lastModifiedTime)
        {
            this.isLink = false;
            this.linkNum = 1;
            this.isDir = false;
            this.fileSize = 0L;
            this.type = -1;
            this.rawRep = "";
            this.fileName = name;
            this.fileSize = size;
            this.isDir = isDir;
            this.lastModifiedTime = lastModifiedTime;
        }

        internal FTPFile(string name, long size, bool isDir, DateTime lastModifiedTime, string path)
        {
            this.isLink = false;
            this.linkNum = 1;
            this.isDir = false;
            this.fileSize = 0L;
            this.type = -1;
            this.rawRep = "";
            this.fileName = name;
            this.fileSize = size;
            this.isDir = isDir;
            this.lastModifiedTime = lastModifiedTime;
            this.filePath = path;
        }

        public FTPFile(int type, string raw, string name, long size, bool isDir, ref DateTime lastModifiedTime)
        {
            this.isLink = false;
            this.linkNum = 1;
            this.isDir = false;
            this.fileSize = 0L;
            this.type = type;
            this.rawRep = raw;
            this.fileName = name;
            this.fileSize = size;
            this.isDir = isDir;
            this.lastModifiedTime = lastModifiedTime;
        }

        internal void ApplyTimeDifference(TimeSpan difference)
        {
            this.lastModifiedTime -= difference;
        }

        internal void SetLastModified(DateTime time, TimeSpan timeDifference)
        {
            this.lastModifiedTime = time;
            this.ApplyTimeDifference(timeDifference);
        }

        public override string ToString()
        {
            StringBuilder builder = new StringBuilder(this.rawRep);
            builder.Append("[").Append("Name=").Append(this.fileName).Append(",").Append("Size=").Append(this.fileSize).Append(",").Append("Permissions=").Append(this.filePermissions).Append(",").Append("Owner=").Append(this.fileOwner).Append(",").Append("Group=").Append(this.userGroup).Append(",").Append("Is link=").Append(this.isLink).Append(",").Append("Link count=").Append(this.linkNum).Append(",").Append("Is dir=").Append(this.isDir).Append(",").Append("Linked name=").Append(this.linkedFileName).Append(",").Append("Permissions=").Append(this.filePermissions).Append(",").Append("Last modified=").Append(this.lastModifiedTime.ToString(format, CultureInfo.CurrentCulture.DateTimeFormat)).Append("]");
            return builder.ToString();
        }

        public FTPFile[] Children
        {
            get
            {
                return this.kids;
            }
            set
            {
                this.kids = value;
            }
        }

        public bool Dir
        {
            get
            {
                return this.isDir;
            }
            set
            {
                this.isDir = value;
            }
        }

        public string Group
        {
            get
            {
                return this.userGroup;
            }
            set
            {
                this.userGroup = value;
            }
        }

        public DateTime LastModified
        {
            get
            {
                return this.lastModifiedTime;
            }
            set
            {
                this.lastModifiedTime = value;
            }
        }

        public bool Link
        {
            get
            {
                return this.isLink;
            }
            set
            {
                this.isLink = value;
            }
        }

        public int LinkCount
        {
            get
            {
                return this.linkNum;
            }
            set
            {
                this.linkNum = value;
            }
        }

        public string LinkedName
        {
            get
            {
                return this.linkedFileName;
            }
            set
            {
                this.linkedFileName = value;
            }
        }

        public string Name
        {
            get
            {
                return this.fileName;
            }
            set
            {
                this.fileName = value;
            }
        }

        public string Owner
        {
            get
            {
                return this.fileOwner;
            }
            set
            {
                this.fileOwner = value;
            }
        }

        public string Path
        {
            get
            {
                return this.filePath;
            }
            set
            {
                this.filePath = value;
            }
        }

        public string Permissions
        {
            get
            {
                return this.filePermissions;
            }
            set
            {
                this.filePermissions = value;
            }
        }

        public string Raw
        {
            get
            {
                return this.rawRep;
            }
        }

        public long Size
        {
            get
            {
                return this.fileSize;
            }
            set
            {
                this.fileSize = value;
            }
        }

        public int Type
        {
            get
            {
                return this.type;
            }
        }
    }
}

