namespace EnterpriseDT.Util.Debug
{
    using System;

    public interface CustomLogLevelAppender : Appender
    {
        Level CurrentLevel { get; set; }
    }
}

