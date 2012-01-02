namespace EnterpriseDT.Util.Debug
{
    using System;

    public class LogLevelHelper
    {
        public static EnterpriseDT.Util.Debug.LogLevel GetLogLevel(string level)
        {
            level = level.ToUpper();
            if (level == EnterpriseDT.Util.Debug.LogLevel.Off.ToString().ToUpper())
            {
                return EnterpriseDT.Util.Debug.LogLevel.Off;
            }
            if (level == EnterpriseDT.Util.Debug.LogLevel.Fatal.ToString().ToUpper())
            {
                return EnterpriseDT.Util.Debug.LogLevel.Fatal;
            }
            if (level == EnterpriseDT.Util.Debug.LogLevel.Error.ToString().ToUpper())
            {
                return EnterpriseDT.Util.Debug.LogLevel.Error;
            }
            if (level == EnterpriseDT.Util.Debug.LogLevel.Warning.ToString().ToUpper())
            {
                return EnterpriseDT.Util.Debug.LogLevel.Warning;
            }
            if (level != EnterpriseDT.Util.Debug.LogLevel.Information.ToString().ToUpper())
            {
                if (level == EnterpriseDT.Util.Debug.LogLevel.Debug.ToString().ToUpper())
                {
                    return EnterpriseDT.Util.Debug.LogLevel.Debug;
                }
                if (level == EnterpriseDT.Util.Debug.LogLevel.All.ToString().ToUpper())
                {
                    return EnterpriseDT.Util.Debug.LogLevel.All;
                }
            }
            return EnterpriseDT.Util.Debug.LogLevel.Information;
        }
    }
}

