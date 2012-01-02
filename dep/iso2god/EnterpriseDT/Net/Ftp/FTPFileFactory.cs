namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Text;

    public class FTPFileFactory
    {
        private Logger log;
        private OS400FileParser os400;
        internal const string OS400_STR = "OS/400";
        private FTPFileParser parser;
        private CultureInfo parserCulture;
        private bool parserDetected;
        private ArrayList parsers;
        private string system;
        protected TimeSpan timeDiff;
        private UnixFileParser unix;
        internal const string UNIX_STR = "UNIX";
        private bool userSetParser;
        private VMSFileParser vms;
        internal const string VMS_STR = "VMS";
        private WindowsFileParser windows;
        internal const string WINDOWS_STR = "WINDOWS";

        public FTPFileFactory()
        {
            this.log = Logger.GetLogger("FTPFileFactory");
            this.windows = new WindowsFileParser();
            this.unix = new UnixFileParser();
            this.vms = new VMSFileParser();
            this.os400 = new OS400FileParser();
            this.parser = null;
            this.parsers = new ArrayList();
            this.userSetParser = false;
            this.parserDetected = false;
            this.parserCulture = CultureInfo.InvariantCulture;
            this.timeDiff = new TimeSpan();
            this.InitializeParsers();
        }

        public FTPFileFactory(FTPFileParser parser)
        {
            this.log = Logger.GetLogger("FTPFileFactory");
            this.windows = new WindowsFileParser();
            this.unix = new UnixFileParser();
            this.vms = new VMSFileParser();
            this.os400 = new OS400FileParser();
            this.parser = null;
            this.parsers = new ArrayList();
            this.userSetParser = false;
            this.parserDetected = false;
            this.parserCulture = CultureInfo.InvariantCulture;
            this.timeDiff = new TimeSpan();
            this.InitializeParsers();
            this.parser = parser;
        }

        public FTPFileFactory(string system)
        {
            this.log = Logger.GetLogger("FTPFileFactory");
            this.windows = new WindowsFileParser();
            this.unix = new UnixFileParser();
            this.vms = new VMSFileParser();
            this.os400 = new OS400FileParser();
            this.parser = null;
            this.parsers = new ArrayList();
            this.userSetParser = false;
            this.parserDetected = false;
            this.parserCulture = CultureInfo.InvariantCulture;
            this.timeDiff = new TimeSpan();
            this.InitializeParsers();
            this.SetParser(system);
        }

        public void AddParser(FTPFileParser parser)
        {
            this.parsers.Add(parser);
        }

        private void DetectParser(string[] files)
        {
            if (this.parser.IsValidFormat(files))
            {
                this.log.Debug("Confirmed format " + this.parser.ToString());
                this.parserDetected = true;
            }
            else
            {
                IEnumerator enumerator = this.parsers.GetEnumerator();
                while (enumerator.MoveNext())
                {
                    FTPFileParser current = (FTPFileParser) enumerator.Current;
                    if (current.IsValidFormat(files))
                    {
                        this.parser = current;
                        this.log.Debug("Detected format " + this.parser.ToString());
                        this.parserDetected = true;
                        return;
                    }
                }
                this.parser = this.unix;
                this.log.Warn("Could not detect format. Using default " + this.parser.ToString());
            }
        }

        [Obsolete("Use the System property.")]
        public string GetSystem()
        {
            return this.system;
        }

        private void InitializeParsers()
        {
            this.parsers.Add(this.unix);
            this.parsers.Add(this.windows);
            this.parsers.Add(this.os400);
            this.parsers.Add(this.vms);
            this.parser = this.unix;
        }

        public virtual FTPFile[] Parse(string[] fileStrings)
        {
            this.log.Debug("Parse() called using culture: " + this.parserCulture.EnglishName);
            FTPFile[] sourceArray = new FTPFile[fileStrings.Length];
            if (fileStrings.Length == 0)
            {
                return sourceArray;
            }
            if (!this.userSetParser && !this.parserDetected)
            {
                this.DetectParser(fileStrings);
            }
            int length = 0;
            for (int i = 0; i < fileStrings.Length; i++)
            {
                if ((fileStrings[i] != null) && (fileStrings[i].Trim().Length != 0))
                {
                    try
                    {
                        FTPFile file = null;
                        if (this.parser.IsMultiLine())
                        {
                            StringBuilder builder = new StringBuilder(fileStrings[i]);
                            while (((i + 1) < fileStrings.Length) && (fileStrings[i + 1].IndexOf(';') < 0))
                            {
                                builder.Append(" ").Append(fileStrings[i + 1]);
                                i++;
                            }
                            file = this.parser.Parse(builder.ToString());
                        }
                        else
                        {
                            file = this.parser.Parse(fileStrings[i]);
                        }
                        if (file != null)
                        {
                            if (this.timeDiff.Ticks != 0L)
                            {
                                file.ApplyTimeDifference(this.timeDiff);
                            }
                            sourceArray[length++] = file;
                        }
                    }
                    catch (RestartParsingException)
                    {
                        this.log.Debug("Restarting parsing from first entry in list");
                        i = -1;
                        length = 0;
                    }
                }
            }
            FTPFile[] destinationArray = new FTPFile[length];
            Array.Copy(sourceArray, 0, destinationArray, 0, length);
            return destinationArray;
        }

        public void SetParser(string system)
        {
            this.parserDetected = false;
            this.system = (system != null) ? system.Trim() : null;
            if (system != null)
            {
                if (system.ToUpper().StartsWith("WINDOWS"))
                {
                    this.log.Debug("Selected Windows parser");
                    this.parser = this.windows;
                }
                else if (system.ToUpper().IndexOf("UNIX") >= 0)
                {
                    this.log.Debug("Selected UNIX parser");
                    this.parser = this.unix;
                }
                else if (system.ToUpper().IndexOf("VMS") >= 0)
                {
                    this.log.Debug("Selected VMS parser");
                    this.parser = this.vms;
                }
                else if (system.ToUpper().IndexOf("OS/400") >= 0)
                {
                    this.log.Debug("Selected OS/400 parser");
                    this.parser = this.os400;
                }
                else
                {
                    this.parser = this.unix;
                    this.log.Warn("Unknown SYST '" + system + "' - defaulting to Unix parsing");
                }
            }
            else
            {
                this.parser = this.unix;
                this.log.Debug("Defaulting to Unix parsing");
            }
        }

        [DefaultValue((string) null)]
        public FTPFileParser FileParser
        {
            get
            {
                return this.parser;
            }
            set
            {
                this.parser = value;
                if (value != null)
                {
                    this.userSetParser = true;
                }
                else
                {
                    this.userSetParser = false;
                    this.SetParser(this.system);
                }
            }
        }

        public bool ParserSetExplicitly
        {
            get
            {
                return this.userSetParser;
            }
        }

        public CultureInfo ParsingCulture
        {
            get
            {
                return this.parserCulture;
            }
            set
            {
                this.parserCulture = value;
                this.windows.ParsingCulture = value;
                this.unix.ParsingCulture = value;
                this.vms.ParsingCulture = value;
                this.os400.ParsingCulture = value;
                if (this.parser != null)
                {
                    this.parser.ParsingCulture = value;
                }
            }
        }

        [DefaultValue((string) null)]
        public string System
        {
            get
            {
                return this.system;
            }
            set
            {
                this.SetParser(value);
            }
        }

        public TimeSpan TimeDifference
        {
            get
            {
                return this.timeDiff;
            }
            set
            {
                this.timeDiff = value;
            }
        }

        public bool TimeIncludesSeconds
        {
            get
            {
                return ((this.parser != null) && this.parser.TimeIncludesSeconds);
            }
        }

        public VMSFileParser VMSParser
        {
            get
            {
                return this.vms;
            }
        }
    }
}

