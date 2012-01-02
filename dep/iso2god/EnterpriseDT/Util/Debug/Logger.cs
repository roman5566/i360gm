namespace EnterpriseDT.Util.Debug
{
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Globalization;
    using System.Reflection;
    using System.Runtime.CompilerServices;
    using System.Text;

    public class Logger
    {
        private static ArrayList appenders = ArrayList.Synchronized(new ArrayList(2));
        private string clazz;
        private static readonly string format = "d MMM yyyy HH:mm:ss.fff";
        private static Level globalLevel = null;
        private static readonly string LEVEL_PARAM = "edtftp.log.level";
        private static Hashtable loggers = Hashtable.Synchronized(new Hashtable(10));
        private static StandardOutputAppender mainConsoleAppender = null;
        private static FileAppender mainFileAppender = null;
        private static TraceAppender mainTraceAppender = null;
        private static bool showClassNames = true;
        private static bool showTimestamp = true;
        private DateTime ts;

        public static  event LogMessageHandler LogMessageReceived;

        static Logger()
        {
            string str = null;
            try
            {
                str = ConfigurationSettings.AppSettings[LEVEL_PARAM];
            }
            catch (Exception exception)
            {
                Console.WriteLine("WARN: Failure reading configuration file: " + exception.Message);
            }
            if (str != null)
            {
                globalLevel = Level.GetLevel(str);
                if (globalLevel == null)
                {
                    try
                    {
                        EnterpriseDT.Util.Debug.LogLevel level = (EnterpriseDT.Util.Debug.LogLevel) Enum.Parse(typeof(EnterpriseDT.Util.Debug.LogLevel), str, true);
                        globalLevel = Level.GetLevel(level);
                    }
                    catch (Exception)
                    {
                    }
                }
            }
            if (globalLevel == null)
            {
                globalLevel = Level.OFF;
                if (str != null)
                {
                    Console.Out.WriteLine("WARN: '" + LEVEL_PARAM + "' configuration property invalid. Unable to parse '" + str + "' - logging switched off");
                }
            }
        }

        private Logger(string clazz)
        {
            this.clazz = clazz;
        }

        public static void AddAppender(Appender newAppender)
        {
            appenders.Add(newAppender);
            if ((newAppender is FileAppender) && (mainFileAppender == null))
            {
                mainFileAppender = (FileAppender) newAppender;
            }
            if ((newAppender is StandardOutputAppender) && (mainConsoleAppender == null))
            {
                mainConsoleAppender = (StandardOutputAppender) newAppender;
            }
            if ((newAppender is TraceAppender) && (mainTraceAppender == null))
            {
                mainTraceAppender = (TraceAppender) newAppender;
            }
        }

        public static void ClearAppenders()
        {
            lock (appenders.SyncRoot)
            {
                for (int i = 0; i < appenders.Count; i++)
                {
                    Appender appender = (Appender) appenders[i];
                    try
                    {
                        appender.Close();
                    }
                    catch (Exception)
                    {
                    }
                }
            }
            appenders.Clear();
        }

        public virtual void Debug(string message)
        {
            this.Log(Level.DEBUG, message, null);
        }

        public virtual void Debug(string message, params object[] args)
        {
            if (this.IsEnabledFor(Level.DEBUG))
            {
                this.Log(Level.DEBUG, string.Format(message, args), null);
            }
        }

        public virtual void Debug(string message, Exception t)
        {
            this.Log(Level.DEBUG, message, new object[] { t });
        }

        private void DumpValue(object value, StringBuilder valueStr, string indent)
        {
            if (value == null)
            {
                valueStr.Append("(null)");
            }
            else if (value.GetType().IsArray && value.GetType().GetElementType().IsPrimitive)
            {
                int num = 0;
                Array array = (Array) value;
                if (array.Length > 0x10)
                {
                    valueStr.Append("(").Append(array.Length).Append(" items)").Append("\n").Append(indent).Append("  ");
                }
                foreach (object obj2 in array)
                {
                    if (obj2 is byte)
                    {
                        valueStr.Append(((byte) obj2).ToString("X2"));
                    }
                    else
                    {
                        valueStr.Append(obj2);
                    }
                    num++;
                    if ((num % 0x10) != 0)
                    {
                        valueStr.Append(" ");
                    }
                    else
                    {
                        valueStr.Append("\n").Append(indent).Append("  ");
                    }
                }
            }
            else if (value is IDictionary)
            {
                IDictionary dictionary = (IDictionary) value;
                bool flag = dictionary.Count > 1;
                if (flag)
                {
                    valueStr.Append("(").Append(dictionary.Count).Append(" items)");
                }
                foreach (object obj3 in dictionary.Keys)
                {
                    if (flag)
                    {
                        valueStr.Append("\n").Append(indent);
                    }
                    valueStr.Append(obj3).Append("=");
                    this.DumpValue(dictionary[obj3], valueStr, indent + "  ");
                }
            }
            else if (value is StringDictionary)
            {
                StringDictionary dictionary2 = (StringDictionary) value;
                bool flag2 = dictionary2.Count > 1;
                if (flag2)
                {
                    valueStr.Append("(").Append(dictionary2.Count).Append(" items)");
                }
                foreach (string str in dictionary2.Keys)
                {
                    if (flag2)
                    {
                        valueStr.Append("\n").Append(indent);
                    }
                    valueStr.Append(str).Append("=");
                    this.DumpValue(dictionary2[str], valueStr, indent + "  ");
                }
            }
            else if (!(value is string) && (value is IEnumerable))
            {
                bool flag3 = true;
                if (value is ICollection)
                {
                    ICollection is2 = (ICollection) value;
                    flag3 = is2.Count > 1;
                    if (flag3)
                    {
                        valueStr.Append("(").Append(is2.Count).Append(" items)");
                    }
                }
                foreach (object obj4 in (IEnumerable) value)
                {
                    if (flag3)
                    {
                        valueStr.Append("\n").Append(indent);
                    }
                    this.DumpValue(obj4, valueStr, indent + "  ");
                }
            }
            else
            {
                valueStr.Append(value.ToString());
            }
        }

        public virtual void Error(Exception t)
        {
            this.Log(Level.ERROR, t.Message, new object[] { t });
        }

        public virtual void Error(string message)
        {
            this.Log(Level.ERROR, message, null);
        }

        public virtual void Error(string message, Exception t)
        {
            this.Log(Level.ERROR, message, new object[] { t });
        }

        public virtual void Error(string message, Exception t, params object[] args)
        {
            this.Log(Level.ERROR, string.Format(message, args), new object[] { t });
        }

        public virtual void Fatal(string message)
        {
            this.Log(Level.FATAL, message, null);
        }

        public virtual void Fatal(string message, Exception t)
        {
            this.Log(Level.FATAL, message, new object[] { t });
        }

        private ArrayList GetAllProperties(Type t)
        {
            ArrayList list = new ArrayList();
            while (t != typeof(object))
            {
                list.AddRange(t.GetProperties(BindingFlags.Public | BindingFlags.Instance | BindingFlags.DeclaredOnly));
                t = t.BaseType;
            }
            return list;
        }

        public static Logger GetLogger(string clazz)
        {
            Logger logger = (Logger) loggers[clazz];
            if (logger == null)
            {
                logger = new Logger(clazz);
                loggers[clazz] = logger;
            }
            return logger;
        }

        public static Logger GetLogger(Type clazz)
        {
            return GetLogger(clazz.FullName);
        }

        public virtual void Info(string message)
        {
            this.Log(Level.INFO, message, null);
        }

        public virtual void Info(string message, params object[] args)
        {
            if (this.IsEnabledFor(Level.INFO))
            {
                this.Log(Level.INFO, string.Format(message, args), null);
            }
        }

        public virtual void Info(string message, Exception t)
        {
            this.Log(Level.INFO, message, new object[] { t });
        }

        public virtual bool IsEnabledFor(Level level)
        {
            if (globalLevel.IsGreaterOrEqual(level))
            {
                return true;
            }
            lock (appenders.SyncRoot)
            {
                foreach (Appender appender in appenders)
                {
                    if (appender is CustomLogLevelAppender)
                    {
                        CustomLogLevelAppender appender2 = (CustomLogLevelAppender) appender;
                        if (appender2.CurrentLevel.IsGreaterOrEqual(level))
                        {
                            return true;
                        }
                    }
                }
            }
            return false;
        }

        public virtual void Log(Level level, string message, params object[] args)
        {
            if (LogMessageReceived != null)
            {
                LogMessageReceived(this, new LogMessageEventArgs(this.clazz, level, message, args));
            }
            if (this.IsEnabledFor(level))
            {
                if (((args != null) && (args.Length == 1)) && (args[0] is Exception))
                {
                    this.OurLog(level, message, (Exception) args[0]);
                }
                else if (args != null)
                {
                    this.OurLog(level, string.Format(message, args), null);
                }
                else
                {
                    this.OurLog(level, message, null);
                }
            }
        }

        public void LogObject(Level level, string prefix, object obj)
        {
            if (this.IsEnabledFor(level))
            {
                if (obj == null)
                {
                    this.Log(level, prefix + "(null)", null);
                }
                obj.GetType();
                bool flag = true;
                PropertyInfo[] properties = obj.GetType().GetProperties(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance);
                foreach (PropertyInfo info in properties)
                {
                    if (this.RequiresLongFormat(info, obj))
                    {
                        flag = false;
                        break;
                    }
                }
                StringBuilder valueStr = new StringBuilder();
                foreach (PropertyInfo info2 in properties)
                {
                    object obj2 = info2.GetValue(obj, null);
                    if (valueStr.Length > 0)
                    {
                        valueStr.Append(flag ? ", " : "\n  ");
                    }
                    valueStr.Append(info2.Name).Append("=");
                    this.DumpValue(obj2, valueStr, "    ");
                }
                this.Log(level, prefix + valueStr, null);
            }
        }

        private void OurLog(Level level, string message, Exception t)
        {
            this.ts = DateTime.Now;
            string str = this.ts.ToString(format, CultureInfo.CurrentCulture.DateTimeFormat);
            StringBuilder builder = new StringBuilder(level.ToString());
            if (showClassNames)
            {
                builder.Append(" [");
                builder.Append(this.clazz);
                builder.Append("]");
            }
            if (showTimestamp)
            {
                builder.Append(" ");
                builder.Append(str);
            }
            builder.Append(" : ");
            string str2 = builder.ToString();
            builder.Append(message);
            if (t != null)
            {
                builder.Append(" : ").Append(t.GetType().FullName).Append(": ").Append(t.Message);
            }
            if (appenders.Count == 0)
            {
                Console.Out.WriteLine(builder.ToString());
                if (t != null)
                {
                    if (t.StackTrace != null)
                    {
                        foreach (string str3 in t.StackTrace.Replace("\r", "").Split(new char[] { '\n' }))
                        {
                            this.OurLog(level, str2 + str3, null);
                        }
                    }
                    if (t.InnerException != null)
                    {
                        Console.Out.WriteLine(string.Format("{0}CAUSED BY - {1}: {2}", str2, t.InnerException.GetType().FullName, t.InnerException.Message));
                        if (t.InnerException.StackTrace != null)
                        {
                            foreach (string str4 in t.InnerException.StackTrace.Replace("\r", "").Split(new char[] { '\n' }))
                            {
                                this.OurLog(level, str2 + str4, null);
                            }
                        }
                    }
                }
            }
            else
            {
                bool flag = globalLevel.IsGreaterOrEqual(level);
                lock (appenders.SyncRoot)
                {
                    for (int i = 0; i < appenders.Count; i++)
                    {
                        Appender appender = (Appender) appenders[i];
                        bool flag2 = false;
                        if (appender is CustomLogLevelAppender)
                        {
                            CustomLogLevelAppender appender2 = (CustomLogLevelAppender) appender;
                            flag2 = appender2.CurrentLevel.IsGreaterOrEqual(level);
                        }
                        if (flag || flag2)
                        {
                            appender.Log(str2 + message);
                            if (t != null)
                            {
                                appender.Log(str2 + t.GetType().FullName + ": " + t.Message);
                                if (t.StackTrace != null)
                                {
                                    foreach (string str5 in t.StackTrace.Replace("\r", "").Split(new char[] { '\n' }))
                                    {
                                        appender.Log(str2 + str5);
                                    }
                                }
                                if (t.InnerException != null)
                                {
                                    appender.Log(str2 + "CAUSED BY - " + t.InnerException.GetType().FullName + ": " + t.Message);
                                    if (t.InnerException.StackTrace != null)
                                    {
                                        foreach (string str6 in t.InnerException.StackTrace.Replace("\r", "").Split(new char[] { '\n' }))
                                        {
                                            appender.Log(str2 + str6);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        public static void RemoveAppender(Appender appender)
        {
            appenders.Remove(appender);
            if (appender == mainFileAppender)
            {
                mainFileAppender = null;
            }
            if (appender == mainConsoleAppender)
            {
                mainConsoleAppender = null;
            }
            if (appender == mainTraceAppender)
            {
                mainTraceAppender = null;
            }
        }

        private bool RequiresLongFormat(PropertyInfo p, object obj)
        {
            object obj2 = p.GetValue(obj, null);
            if (((obj2 == null) || (obj2 is string)) || obj2.GetType().IsPrimitive)
            {
                return false;
            }
            if (obj2.GetType().IsArray && obj2.GetType().GetElementType().IsPrimitive)
            {
                return (((Array) obj2).Length > 0x10);
            }
            if (obj2 is StringDictionary)
            {
                return (((StringDictionary) obj2).Count > 1);
            }
            if (obj2 is ICollection)
            {
                return (((ICollection) obj2).Count > 1);
            }
            return typeof(IEnumerable).IsAssignableFrom(p.PropertyType);
        }

        public static void Shutdown()
        {
            ClearAppenders();
            loggers.Clear();
        }

        public virtual void Warn(string message)
        {
            this.Log(Level.WARN, message, null);
        }

        public virtual void Warn(string message, Exception t)
        {
            this.Log(Level.WARN, message, new object[] { t });
        }

        public static Level CurrentLevel
        {
            get
            {
                return globalLevel;
            }
            set
            {
                globalLevel = value;
            }
        }

        public virtual bool DebugEnabled
        {
            get
            {
                return this.IsEnabledFor(Level.DEBUG);
            }
        }

        public virtual bool InfoEnabled
        {
            get
            {
                return this.IsEnabledFor(Level.INFO);
            }
        }

        public static bool LogToConsole
        {
            get
            {
                return (mainConsoleAppender != null);
            }
            set
            {
                if (value)
                {
                    if (mainConsoleAppender == null)
                    {
                        AddAppender(new StandardOutputAppender());
                    }
                }
                else if (mainConsoleAppender != null)
                {
                    RemoveAppender(mainConsoleAppender);
                }
            }
        }

        public static bool LogToTrace
        {
            get
            {
                return (mainTraceAppender != null);
            }
            set
            {
                if (value)
                {
                    if (mainTraceAppender == null)
                    {
                        AddAppender(new TraceAppender());
                    }
                }
                else if (mainTraceAppender != null)
                {
                    RemoveAppender(mainTraceAppender);
                }
            }
        }

        public static string PrimaryLogFile
        {
            get
            {
                if (mainFileAppender == null)
                {
                    return null;
                }
                return mainFileAppender.FileName;
            }
            set
            {
                string str = (mainFileAppender != null) ? mainFileAppender.FileName : null;
                if (str != value)
                {
                    if (mainFileAppender != null)
                    {
                        RemoveAppender(mainFileAppender);
                    }
                    if (value != null)
                    {
                        AddAppender(new FileAppender(value));
                    }
                }
            }
        }

        public static bool ShowClassNames
        {
            get
            {
                return showClassNames;
            }
            set
            {
                showClassNames = value;
            }
        }

        public static bool ShowTimestamp
        {
            get
            {
                return showTimestamp;
            }
            set
            {
                showTimestamp = value;
            }
        }
    }
}

