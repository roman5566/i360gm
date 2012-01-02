namespace EnterpriseDT.Util.Debug
{
    using System;

    public class LogMessageEventArgs : EventArgs
    {
        private object[] args;
        private System.Exception e;
        private Level level;
        private string loggerName;
        private string text;

        internal LogMessageEventArgs(string loggerName, Level level, string text, params object[] args)
        {
            this.loggerName = loggerName;
            this.level = level;
            this.text = text;
            this.args = args;
            if (((args != null) && (args.Length == 1)) && (args[0] is System.Exception))
            {
                this.e = (System.Exception) args[0];
            }
        }

        public object[] Arguments
        {
            get
            {
                return this.args;
            }
        }

        public System.Exception Exception
        {
            get
            {
                return this.e;
            }
        }

        public string FormattedText
        {
            get
            {
                if (this.args != null)
                {
                    return string.Format(this.text, this.args);
                }
                return this.text;
            }
        }

        public string LoggerName
        {
            get
            {
                return this.loggerName;
            }
        }

        public Level LogLevel
        {
            get
            {
                return this.level;
            }
        }

        public string Text
        {
            get
            {
                return this.text;
            }
        }
    }
}

