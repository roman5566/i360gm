namespace Chilano.Iso2God.Ftp
{
    using Chilano.Iso2God;
    using System;

    public class FtpUploaderArgs
    {
        public string ContainerID;
        public string Ip;
        public string Pass;
        public IsoEntryPlatform Platform;
        public string Port;
        public string SourcePath;
        public string TitleID;
        public string User;

        public FtpUploaderArgs(string Ip, string User, string Pass, string Port, string TitleID, string ContainerID, string SourcePath, IsoEntryPlatform Platform)
        {
            this.Ip = Ip;
            this.User = User;
            this.Pass = Pass;
            this.Port = Port;
            this.TitleID = TitleID;
            this.ContainerID = ContainerID;
            this.SourcePath = SourcePath;
            this.Platform = Platform;
        }
    }
}

