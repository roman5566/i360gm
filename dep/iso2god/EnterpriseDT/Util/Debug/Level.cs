namespace EnterpriseDT.Util.Debug
{
    using System;

    public class Level
    {
        public static Level ALL = new Level(EnterpriseDT.Util.Debug.LogLevel.All, "ALL");
        private const string ALL_STR = "ALL";
        public static Level DEBUG = new Level(EnterpriseDT.Util.Debug.LogLevel.Debug, "DEBUG");
        private const string DEBUG_STR = "DEBUG";
        public static Level ERROR = new Level(EnterpriseDT.Util.Debug.LogLevel.Error, "ERROR");
        private const string ERROR_STR = "ERROR";
        public static Level FATAL = new Level(EnterpriseDT.Util.Debug.LogLevel.Fatal, "FATAL");
        private const string FATAL_STR = "FATAL";
        public static Level INFO = new Level(EnterpriseDT.Util.Debug.LogLevel.Information, "INFO");
        private const string INFO_STR = "INFO";
        private EnterpriseDT.Util.Debug.LogLevel level = EnterpriseDT.Util.Debug.LogLevel.Off;
        private string levelStr;
        public static Level OFF = new Level(EnterpriseDT.Util.Debug.LogLevel.Off, "OFF");
        private const string OFF_STR = "OFF";
        public static Level WARN = new Level(EnterpriseDT.Util.Debug.LogLevel.Warning, "WARN");
        private const string WARN_STR = "WARN";

        private Level(EnterpriseDT.Util.Debug.LogLevel level, string levelStr)
        {
            this.level = level;
            this.levelStr = levelStr;
        }

        public EnterpriseDT.Util.Debug.LogLevel GetLevel()
        {
            return this.level;
        }

        public static Level GetLevel(EnterpriseDT.Util.Debug.LogLevel level)
        {
            switch (level)
            {
                case EnterpriseDT.Util.Debug.LogLevel.Off:
                    return OFF;

                case EnterpriseDT.Util.Debug.LogLevel.Fatal:
                    return FATAL;

                case EnterpriseDT.Util.Debug.LogLevel.Error:
                    return ERROR;

                case EnterpriseDT.Util.Debug.LogLevel.Warning:
                    return WARN;

                case EnterpriseDT.Util.Debug.LogLevel.Information:
                    return INFO;

                case EnterpriseDT.Util.Debug.LogLevel.Debug:
                    return DEBUG;

                case EnterpriseDT.Util.Debug.LogLevel.All:
                    return ALL;
            }
            return OFF;
        }

        public static Level GetLevel(string level)
        {
            switch (level.ToUpper())
            {
                case "OFF":
                    return OFF;

                case "FATAL":
                    return FATAL;

                case "ERROR":
                    return ERROR;

                case "WARN":
                    return WARN;

                case "INFO":
                    return INFO;

                case "DEBUG":
                    return DEBUG;

                case "ALL":
                    return ALL;
            }
            return null;
        }

        public bool IsGreaterOrEqual(Level l)
        {
            return (this.level >= l.level);
        }

        public override string ToString()
        {
            return this.levelStr;
        }
    }
}

