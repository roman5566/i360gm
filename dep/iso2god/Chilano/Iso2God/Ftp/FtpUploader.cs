namespace Chilano.Iso2God.Ftp
{
    using Chilano.Iso2God;
    using EnterpriseDT.Net.Ftp;
    using System;
    using System.Collections.Generic;
    using System.ComponentModel;
    using System.IO;

    public class FtpUploader : BackgroundWorker
    {
        private FtpUploaderArgs args;
        public bool Debug;
        public List<Exception> Errors = new List<Exception>();
        private FTPConnection ftp;

        public FtpUploader()
        {
            base.WorkerReportsProgress = true;
            base.WorkerSupportsCancellation = false;
            base.DoWork += new DoWorkEventHandler(this.FtpUploader_DoWork);
        }

        private void clearDir(string dir)
        {
            try
            {
                this.ftp.ChangeWorkingDirectory(dir);
                foreach (FTPFile file in this.ftp.GetFileInfos())
                {
                    this.ftp.DeleteFile(file.Name);
                }
                this.ftp.ChangeWorkingDirectoryUp();
            }
            catch (FTPException exception)
            {
                this.Errors.Add(exception);
            }
        }

        private bool dirExists(string dir)
        {
            try
            {
                foreach (FTPFile file in this.ftp.GetFileInfos())
                {
                    if (file.Dir && file.Name.StartsWith(dir))
                    {
                        return true;
                    }
                }
                return false;
            }
            catch (FTPException exception)
            {
                this.Errors.Add(exception);
                return false;
            }
        }

        private bool fileExists(string file)
        {
            try
            {
                foreach (FTPFile file2 in this.ftp.GetFileInfos())
                {
                    if (!file2.Dir && file2.Name.StartsWith(file))
                    {
                        return true;
                    }
                }
            }
            catch (FTPException exception)
            {
                this.Errors.Add(exception);
            }
            return false;
        }

        private void FtpUploader_DoWork(object sender, DoWorkEventArgs e)
        {
            this.ftp = new FTPConnection();
            try
            {
                this.args = (FtpUploaderArgs) e.Argument;
            }
            catch
            {
                this.Errors.Add(new ArgumentNullException("FtpUploader must be passed an instance of FtpUploaderArgs."));
                return;
            }
            string dir = (this.args.Platform == IsoEntryPlatform.Xbox360) ? "00007000" : "00005000";
            string str2 = string.Concat(new object[] { this.args.SourcePath, this.args.TitleID, Path.DirectorySeparatorChar, dir, Path.DirectorySeparatorChar });
            this.ftp.ServerAddress = this.args.Ip;
            this.ftp.UserName = this.args.User;
            this.ftp.Password = this.args.Pass;
            this.ftp.AutoLogin = true;
            try
            {
                this.ftp.Connect();
            }
            catch (Exception exception)
            {
                this.Errors.Add(exception);
                return;
            }
            this.ftp.ChangeWorkingDirectory("Hdd1/Content/0000000000000000");
            if (!this.dirExists(this.args.TitleID))
            {
                this.ftp.CreateDirectory(this.args.TitleID);
            }
            this.ftp.ChangeWorkingDirectory(this.args.TitleID);
            if (!this.dirExists(dir))
            {
                this.ftp.CreateDirectory(dir);
            }
            this.ftp.ChangeWorkingDirectory(dir);
            if (!this.dirExists(this.args.ContainerID + ".data"))
            {
                this.ftp.CreateDirectory(this.args.ContainerID + ".data");
            }
            else
            {
                this.clearDir(this.args.ContainerID + ".data");
            }
            if (this.fileExists(this.args.ContainerID))
            {
                this.ftp.DeleteFile(this.args.ContainerID);
            }
            base.ReportProgress(1, "Uploading GOD header...");
            FileStream srcStream = new FileStream(str2 + this.args.ContainerID, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            this.ftp.UploadStream(srcStream, this.args.ContainerID);
            this.ftp.ChangeWorkingDirectory(this.args.ContainerID + ".data");
            int percentProgress = 0;
            string[] files = Directory.GetFiles(str2 + this.args.ContainerID + ".data");
            foreach (string str3 in files)
            {
                string remoteFile = str3.Substring(str3.LastIndexOf('\\') + 1);
                base.ReportProgress(percentProgress, "Uploading '" + remoteFile + "'...");
                FileStream stream2 = new FileStream(str3, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                this.ftp.UploadStream(stream2, remoteFile);
                percentProgress += (int) Math.Floor((double) ((1f / ((float) files.Length)) * 100f));
            }
            this.ftp.Close();
            this.Errors.Clear();
            base.ReportProgress(100, "Uploaded");
        }
    }
}

