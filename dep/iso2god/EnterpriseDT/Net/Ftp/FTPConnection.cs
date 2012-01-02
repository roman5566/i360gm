namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Runtime.CompilerServices;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    [ToolboxBitmap(typeof(FTPConnection)), DefaultProperty("Protocol")]
    public class FTPConnection : Component, IFTPComponent
    {
        protected string accountInfoStr;
        private IFileTransferClient activeClient;
        protected bool areEventsEnabled;
        protected object clientLock;
        private Container components;
        protected long currentFileSize;
        protected const string DEFAULT_WORKING_DIRECTORY = null;
        protected FTPTransferType fileTransferType;
        protected FTPClient ftpClient;
        protected Control guiControl;
        protected bool haveQueriedForControl;
        private static int instanceCount = 0;
        private static object instanceCountMutex = new object();
        private int instanceNumber;
        private static FTPSemaphore invokeSemaphore = new FTPSemaphore(0x3e8);
        protected bool isTransferringData;
        protected bool lastTransferCancel;
        protected string localDir;
        private Logger log;
        protected string loginPassword;
        protected string loginUserName;
        private string name;
        protected string remoteDir;
        protected bool useAutoLogin;
        protected bool useGuiThread;

        [Category("Transfer"), Description("Occurs every time 'TransferNotifyInterval' bytes have been transferred.")]
        public event BytesTransferredHandler BytesTransferred;

        [Category("Connection"), Description("Occurs when the component has closed its connection to the server.")]
        public event FTPConnectionEventHandler Closed;

        [Description("Occurs when the component is about to close its connection to the server."), Category("Connection")]
        public event FTPConnectionEventHandler Closing;

        [Category("Commands"), Description("Occurs when a command is sent to the server.")]
        public event FTPMessageHandler CommandSent;

        [Category("Connection"), Description("Occurs when the component has connected to the server.")]
        public event FTPConnectionEventHandler Connected;

        [Category("Connection"), Description("Occurs when the component is connecting to the server.")]
        public event FTPConnectionEventHandler Connecting;

        [Category("Directory"), Description("Occurs when a local directory has been created on the server.")]
        public event FTPDirectoryEventHandler CreatedDirectory;

        [Category("Directory"), Description("Occurs when a directory is about to be created on the server.")]
        public event FTPDirectoryEventHandler CreatingDirectory;

        [Description("Occurs when a file has been deleted from the server."), Category("File")]
        public event FTPFileTransferEventHandler Deleted;

        [Category("Directory"), Description("Occurs when a local directory has been deleted on the server.")]
        public event FTPDirectoryEventHandler DeletedDirectory;

        [Description("Occurs when a file is about to be deleted from the server."), Category("File")]
        public event FTPFileTransferEventHandler Deleting;

        [Description("Occurs when a directory is about to be deleted on the server."), Category("Directory")]
        public event FTPDirectoryEventHandler DeletingDirectory;

        [EditorBrowsable(EditorBrowsableState.Never), Obsolete("Use ServerDirectoryChanged"), Browsable(false)]
        public event FTPDirectoryEventHandler DirectoryChanged;

        [Browsable(false), Obsolete("Use ServerDirectoryChanging"), EditorBrowsable(EditorBrowsableState.Never)]
        public event FTPDirectoryEventHandler DirectoryChanging;

        [Description("Occurs when a directory listing operations is completed."), Category("Directory")]
        public event FTPDirectoryListEventHandler DirectoryListed;

        [Category("Directory"), Description("Occurs when a directory listing operations is commenced.")]
        public event FTPDirectoryListEventHandler DirectoryListing;

        [Description("Occurs when a file has been downloaded from the server."), Category("File")]
        public event FTPFileTransferEventHandler Downloaded;

        [Description("Occurs when a file is about to be downloaded from the server."), Category("File")]
        public event FTPFileTransferEventHandler Downloading;

        [Description("Occurs when the local directory has been changed."), Category("Directory")]
        public event FTPDirectoryEventHandler LocalDirectoryChanged;

        [Description("Occurs when the local directory is about to be changed."), Category("Directory")]
        public event FTPDirectoryEventHandler LocalDirectoryChanging;

        [Category("Connection"), Description("Occurs when the component has logged in.")]
        public event FTPLogInEventHandler LoggedIn;

        [Description("Occurs when the component is about to log in."), Category("Connection")]
        public event FTPLogInEventHandler LoggingIn;

        [Description("Occurs when a property is changed."), Category("Property Changed")]
        public event PropertyChangedEventHandler PropertyChanged;

        [Category("File"), Description("Occurs when a remote file has been renamed.")]
        public event FTPFileRenameEventHandler RenamedFile;

        [Category("File"), Description("Occurs when a remote file is about to be renamed.")]
        public event FTPFileRenameEventHandler RenamingFile;

        [Description("Occurs when a reply is received from the server."), Category("Commands")]
        public event FTPMessageHandler ReplyReceived;

        [Description("Occurs when the server directory has been changed."), Category("Directory")]
        public event FTPDirectoryEventHandler ServerDirectoryChanged;

        [Description("Occurs when the server directory is about to be changed."), Category("Directory")]
        public event FTPDirectoryEventHandler ServerDirectoryChanging;

        [Category("File"), Description("Occurs when a file has been uploaded to the server.")]
        public event FTPFileTransferEventHandler Uploaded;

        [Description("Occurs when a file is about to be uploaded to the server."), Category("File")]
        public event FTPFileTransferEventHandler Uploading;

        public FTPConnection() : this(new FTPClient())
        {
            this.components = new Container();
        }

        protected internal FTPConnection(FTPClient ftpClient)
        {
            this.components = null;
            this.log = Logger.GetLogger("FTPConnection");
            this.clientLock = new object();
            this.useAutoLogin = true;
            this.areEventsEnabled = true;
            this.isTransferringData = false;
            this.guiControl = null;
            this.haveQueriedForControl = false;
            this.currentFileSize = -1L;
            this.useGuiThread = true;
            this.localDir = null;
            this.remoteDir = null;
            this.lastTransferCancel = false;
            lock (instanceCountMutex)
            {
                this.instanceNumber = instanceCount++;
            }
            this.ftpClient = ftpClient;
            this.activeClient = ftpClient;
            this.ftpClient.AutoPassiveIPSubstitution = true;
            this.ftpClient.BytesTransferred += new BytesTransferredHandler(this.ftpClient_BytesTransferred);
            this.fileTransferType = FTPTransferType.BINARY;
            ftpClient.CommandSent += new FTPMessageHandler(this.ftpClient_CommandSent);
            ftpClient.ReplyReceived += new FTPMessageHandler(this.ftpClient_ReplyReceived);
            ftpClient.ActivePortRange.PropertyChangeHandler = new PropertyChangedEventHandler(this.OnActivePortRangeChanged);
            ftpClient.FileNotFoundMessages.PropertyChangeHandler = new PropertyChangedEventHandler(this.OnFileNotFoundMessagesChanged);
            ftpClient.TransferCompleteMessages.PropertyChangeHandler = new PropertyChangedEventHandler(this.OnFileNotFoundMessagesChanged);
            ftpClient.DirectoryEmptyMessages.PropertyChangeHandler = new PropertyChangedEventHandler(this.OnDirectoryEmptyMessagesChanged);
        }

        public FTPConnection(IContainer container) : this()
        {
            container.Add(this);
        }

        public virtual void CancelResume()
        {
            this.log.Info("Cancel resume");
            this.ActiveClient.CancelResume();
        }

        public virtual void CancelTransfer()
        {
            if (this.isTransferringData)
            {
                this.ActiveClient.CancelTransfer();
                this.lastTransferCancel = true;
            }
            else
            {
                this.log.Debug("CancelTransfer() called while not transfering data");
            }
        }

        [MethodIdentifier(MethodIdentifier.ChangeWorkingDirectory, typeof(string))]
        public bool ChangeWorkingDirectory(string directory)
        {
            lock (this.clientLock)
            {
                string oldDirectory = null;
                if (this.areEventsEnabled && ((this.DirectoryChanging != null) || (this.DirectoryChanged != null)))
                {
                    oldDirectory = this.remoteDir;
                }
                bool wasCancelled = false;
                Exception ex = null;
                try
                {
                    if (this.OnChangingServerDirectory(oldDirectory, directory))
                    {
                        this.ActiveClient.ChDir(directory);
                        this.remoteDir = this.ActiveClient.Pwd();
                    }
                    else
                    {
                        wasCancelled = true;
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnChangedServerDirectory(oldDirectory, this.remoteDir, wasCancelled, ex);
                }
                return !wasCancelled;
            }
        }

        [MethodIdentifier(MethodIdentifier.ChangeWorkingDirectoryUp)]
        public bool ChangeWorkingDirectoryUp()
        {
            lock (this.clientLock)
            {
                string oldDirectory = null;
                if (this.areEventsEnabled && ((this.DirectoryChanging != null) || (this.DirectoryChanged != null)))
                {
                    oldDirectory = this.ServerDirectory;
                }
                bool wasCancelled = false;
                Exception ex = null;
                try
                {
                    if (this.OnChangingServerDirectory(oldDirectory, ".."))
                    {
                        this.ActiveClient.CdUp();
                        this.remoteDir = this.ActiveClient.Pwd();
                    }
                    else
                    {
                        wasCancelled = true;
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnChangedServerDirectory(oldDirectory, this.remoteDir, wasCancelled, ex);
                }
                return !wasCancelled;
            }
        }

        protected internal void CheckConnection(bool shouldBeConnected)
        {
            if (shouldBeConnected && !this.ActiveClient.IsConnected)
            {
                throw new FTPException("The FTP client has not yet connected to the server.  The requested action cannot be performed until after a connection has been established.");
            }
            if (!shouldBeConnected && this.ActiveClient.IsConnected)
            {
                throw new FTPException("The FTP client has already been connected to the server.  The requested action must be performed before a connection is established.");
            }
        }

        protected virtual void CheckFTPType(bool ftpOnly)
        {
            if (ftpOnly)
            {
                if (this.Protocol.Equals(FileTransferProtocol.HTTP))
                {
                    throw new FTPException("This operation is only supported for FTP/FTPS");
                }
                if (this.Protocol.Equals(FileTransferProtocol.SFTP))
                {
                    throw new FTPException("This operation is only supported for FTP/FTPS");
                }
            }
        }

        public void Close()
        {
            this.Close(false);
        }

        [MethodIdentifier(MethodIdentifier.Close, typeof(bool))]
        public virtual void Close(bool abruptClose)
        {
            try
            {
                this.OnClosing();
                this.log.Debug("Closing connection (instance=" + this.instanceNumber + ")");
                if (abruptClose)
                {
                    if (this.isTransferringData)
                    {
                        this.ActiveClient.CancelTransfer();
                    }
                    this.ActiveClient.QuitImmediately();
                }
                else
                {
                    lock (this.clientLock)
                    {
                        this.ActiveClient.Quit();
                    }
                }
            }
            finally
            {
                this.OnClosed();
            }
        }

        [MethodIdentifier(MethodIdentifier.Connect)]
        public virtual void Connect()
        {
            lock (this.clientLock)
            {
                bool flag = false;
                try
                {
                    if (this.LocalDirectory == null)
                    {
                        this.LocalDirectory = Directory.GetCurrentDirectory();
                    }
                    if (this.ServerAddress == null)
                    {
                        throw new FTPException("ServerAddress not set");
                    }
                    this.OnConnecting();
                    this.ActiveClient.Connect();
                    this.log.Debug(string.Concat(new object[] { "Connected to ", this.ServerAddress, " (instance=", this.instanceNumber, ")" }));
                    this.OnConnected(true);
                    flag = this.PerformAutoLogin();
                }
                catch
                {
                    this.OnConnected(this.IsConnected);
                    if (!this.IsConnected)
                    {
                        this.OnClosed();
                    }
                    throw;
                }
                if (flag)
                {
                    this.PostLogin();
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.CreateDirectory, typeof(string))]
        public virtual void CreateDirectory(string directory)
        {
            lock (this.clientLock)
            {
                Exception ex = null;
                bool cancelled = false;
                try
                {
                    if (this.OnCreatingDirectory(directory))
                    {
                        this.ActiveClient.MkDir(directory);
                    }
                    else
                    {
                        cancelled = true;
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnCreatedDirectory(directory, cancelled, ex);
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.DeleteDirectory, typeof(string))]
        public virtual void DeleteDirectory(string directory)
        {
            lock (this.clientLock)
            {
                Exception ex = null;
                bool cancelled = false;
                try
                {
                    if (this.OnDeletingDirectory(directory))
                    {
                        this.ActiveClient.RmDir(directory);
                    }
                    else
                    {
                        cancelled = true;
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnDeletedDirectory(directory, cancelled, ex);
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.DeleteFile, typeof(string))]
        public virtual bool DeleteFile(string remoteFile)
        {
            lock (this.clientLock)
            {
                bool cancelled = false;
                Exception ex = null;
                try
                {
                    if (this.OnDeleting(remoteFile))
                    {
                        this.ActiveClient.Delete(remoteFile);
                    }
                    else
                    {
                        cancelled = true;
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnDeleted(remoteFile, cancelled, ex);
                }
                return !cancelled;
            }
        }

        internal virtual bool DirectoryExists(string dir)
        {
            if (dir == null)
            {
                throw new ArgumentNullException();
            }
            if (dir.Length == 0)
            {
                throw new ArgumentException("Empty string", "dir");
            }
            lock (this.clientLock)
            {
                string remoteDir = this.remoteDir;
                try
                {
                    this.ActiveClient.ChDir(dir);
                }
                catch (Exception)
                {
                    return false;
                }
                if (dir.IndexOf('/') < 0)
                {
                    this.ActiveClient.ChDir("..");
                }
                else
                {
                    this.ActiveClient.ChDir(remoteDir);
                }
                return true;
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                if (this.components != null)
                {
                    this.components.Dispose();
                }
                if (this.IsConnected)
                {
                    this.Close(true);
                }
            }
            base.Dispose(disposing);
        }

        [MethodIdentifier(MethodIdentifier.DownloadByteArray, typeof(string))]
        public virtual byte[] DownloadByteArray(string remoteFile)
        {
            lock (this.clientLock)
            {
                this.lastTransferCancel = false;
                byte[] bytes = null;
                Exception ex = null;
                try
                {
                    if (this.OnDownloading(remoteFile))
                    {
                        try
                        {
                            this.isTransferringData = true;
                            bytes = this.ActiveClient.Get(remoteFile);
                        }
                        finally
                        {
                            this.isTransferringData = false;
                        }
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnDownloaded(bytes, remoteFile, ex);
                }
                return bytes;
            }
        }

        public virtual void DownloadFile(string localPath, string remoteFile)
        {
            this.log.Debug("DownloadFile(" + localPath + "," + remoteFile + ")");
            string str = localPath;
            if (this.localDir != null)
            {
                str = this.RelativePathToAbsolute(this.localDir, localPath);
            }
            lock (this.clientLock)
            {
                Exception ex = null;
                try
                {
                    this.lastTransferCancel = false;
                    if (this.OnDownloading(ref str, remoteFile))
                    {
                        try
                        {
                            this.isTransferringData = true;
                            this.ActiveClient.Get(str, remoteFile);
                        }
                        finally
                        {
                            this.isTransferringData = false;
                        }
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnDownloaded(str, remoteFile, ex);
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.DownloadStream, typeof(Stream), typeof(string))]
        public virtual void DownloadStream(Stream destStream, string remoteFile)
        {
            lock (this.clientLock)
            {
                Exception ex = null;
                try
                {
                    this.lastTransferCancel = false;
                    if (this.OnDownloading(destStream, remoteFile))
                    {
                        try
                        {
                            this.isTransferringData = true;
                            this.ActiveClient.Get(destStream, remoteFile);
                        }
                        finally
                        {
                            this.isTransferringData = false;
                        }
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnDownloaded(destStream, remoteFile, ex);
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.Exists, typeof(string))]
        public virtual bool Exists(string remoteFile)
        {
            lock (this.clientLock)
            {
                return this.ActiveClient.Exists(remoteFile);
            }
        }

        protected internal void ftpClient_BytesTransferred(object sender, BytesTransferredEventArgs e)
        {
            this.OnBytesTransferred(e.RemoteFile, e.ByteCount, e.ResumeOffset);
        }

        protected internal void ftpClient_CommandSent(object sender, FTPMessageEventArgs e)
        {
            this.RaiseCommandSent(e);
        }

        protected internal virtual void ftpClient_ReplyReceived(object sender, FTPMessageEventArgs e)
        {
            this.RaiseReplyReceived(e);
        }

        [MethodIdentifier(MethodIdentifier.GetCommandHelp, typeof(string))]
        public virtual string GetCommandHelp(string command)
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                return this.ftpClient.Help(command);
            }
        }

        [MethodIdentifier(MethodIdentifier.GetFeatures)]
        public virtual string[] GetFeatures()
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                return this.ftpClient.Features();
            }
        }

        [MethodIdentifier(MethodIdentifier.GetFileInfos)]
        public virtual FTPFile[] GetFileInfos()
        {
            return this.GetFileInfos("");
        }

        [MethodIdentifier(MethodIdentifier.GetFileInfos, typeof(string))]
        public virtual FTPFile[] GetFileInfos(string directory)
        {
            lock (this.clientLock)
            {
                FTPFile[] files = null;
                try
                {
                    this.OnDirectoryListing(directory);
                    this.isTransferringData = true;
                    files = this.ActiveClient.DirDetails(directory);
                    if (files == null)
                    {
                        files = new FTPFile[0];
                    }
                }
                finally
                {
                    this.isTransferringData = false;
                    this.OnDirectoryListed(directory, files);
                }
                return files;
            }
        }

        [MethodIdentifier(MethodIdentifier.GetFiles)]
        public virtual string[] GetFiles()
        {
            return this.GetFiles("");
        }

        [MethodIdentifier(MethodIdentifier.GetFiles, typeof(string))]
        public virtual string[] GetFiles(string directory)
        {
            return this.GetFiles(directory, false);
        }

        [MethodIdentifier(MethodIdentifier.GetFiles, typeof(string), typeof(bool))]
        public virtual string[] GetFiles(string directory, bool full)
        {
            string[] strArray2;
            lock (this.clientLock)
            {
                try
                {
                    this.isTransferringData = true;
                    this.log.Debug("Listing directory '" + directory + "'");
                    string[] strArray = this.ActiveClient.Dir(directory, full);
                    this.log.Debug("Listed directory '" + directory + "'");
                    if (((strArray.Length == 0) && (this.LastValidReply != null)) && (this.LastValidReply.ReplyText.ToLower().IndexOf("permission") >= 0))
                    {
                        FTPFile[] fileInfos = this.GetFileInfos(directory);
                        strArray = new string[fileInfos.Length];
                        for (int i = 0; i < fileInfos.Length; i++)
                        {
                            strArray[i] = full ? fileInfos[i].Raw : fileInfos[i].Name;
                        }
                    }
                    strArray2 = strArray;
                }
                finally
                {
                    this.isTransferringData = false;
                }
            }
            return strArray2;
        }

        public override int GetHashCode()
        {
            return this.instanceNumber;
        }

        [MethodIdentifier(MethodIdentifier.GetLastWriteTime, typeof(string))]
        public virtual DateTime GetLastWriteTime(string remoteFile)
        {
            lock (this.clientLock)
            {
                return this.ActiveClient.ModTime(remoteFile);
            }
        }

        [MethodIdentifier(MethodIdentifier.GetSize, typeof(string))]
        public virtual long GetSize(string remoteFile)
        {
            return this.GetSize(remoteFile, true);
        }

        [MethodIdentifier(MethodIdentifier.GetSize, typeof(string), typeof(bool))]
        private long GetSize(string remoteFile, bool throwOnError)
        {
            object obj2;
            long num;
            Monitor.Enter(obj2 = this.clientLock);
            try
            {
                num = this.ActiveClient.Size(remoteFile);
            }
            catch (FTPException exception)
            {
                if (throwOnError)
                {
                    throw;
                }
                if (this.log.IsEnabledFor(Level.WARN))
                {
                    this.log.Warn(string.Concat(new object[] { "Could not get size of file ", remoteFile, " - ", exception.ReplyCode, " ", exception.Message }));
                }
                num = -1L;
            }
            finally
            {
                Monitor.Exit(obj2);
            }
            return num;
        }

        [MethodIdentifier(MethodIdentifier.GetSystemType)]
        public virtual string GetSystemType()
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                return this.ftpClient.GetSystem();
            }
        }

        public virtual string GetURL()
        {
            return this.GetURL(true, true, true);
        }

        public virtual string GetURL(bool includeDirectory, bool includeUserName, bool includePassword)
        {
            if (includePassword && !includeUserName)
            {
                throw new ArgumentException("Cannot include password in URL without also including the user-name");
            }
            StringBuilder builder = new StringBuilder();
            builder.Append("ftp://");
            if (includeUserName)
            {
                builder.Append(this.UserName);
                if (includePassword)
                {
                    builder.Append(":" + this.Password);
                }
                builder.Append("@");
            }
            builder.Append(this.ServerAddress);
            if (this.ServerPort != 0x15)
            {
                builder.Append(":" + this.ServerPort);
            }
            if (includeDirectory)
            {
                if (!this.ServerDirectory.StartsWith("/"))
                {
                    builder.Append("/");
                }
                builder.Append(this.ServerDirectory);
            }
            return builder.ToString();
        }

        [MethodIdentifier(MethodIdentifier.GetWorkingDirectory), Obsolete("Use FTPConnection.ServerDirectory."), EditorBrowsable(EditorBrowsableState.Never)]
        public string GetWorkingDirectory()
        {
            lock (this.clientLock)
            {
                return this.ActiveClient.Pwd();
            }
        }

        protected internal object InvokeDelegate(bool preferGuiThread, bool permitAsync, Delegate del, params object[] args)
        {
            FTPEventArgs args2 = ((args.Length == 2) && (args[1] is FTPEventArgs)) ? ((FTPEventArgs) args[1]) : null;
            if ((this.useGuiThread && preferGuiThread) && ((this.guiControl == null) && !this.haveQueriedForControl))
            {
                try
                {
                    if (base.Container is Control)
                    {
                        this.guiControl = (Control) base.Container;
                    }
                    else
                    {
                        IntPtr mainWindowHandle = Process.GetCurrentProcess().MainWindowHandle;
                        if (mainWindowHandle != IntPtr.Zero)
                        {
                            this.guiControl = Control.FromHandle(mainWindowHandle);
                        }
                    }
                }
                catch (Exception exception)
                {
                    this.log.Log(Level.ALL, "Error while getting GUI control", new object[] { exception });
                }
                finally
                {
                    this.haveQueriedForControl = true;
                }
            }
            if (((this.useGuiThread && preferGuiThread) && ((this.guiControl != null) && this.guiControl.InvokeRequired)) && !this.guiControl.IsDisposed)
            {
                if (args2 != null)
                {
                    args2.IsGuiThread = true;
                }
                invokeSemaphore.WaitOne(0x493e0);
                IAsyncResult asyncResult = this.guiControl.BeginInvoke(new RunDelegateDelegate(this.RunDelegate), new object[] { new RunDelegateArgs(del, args) });
                if (permitAsync)
                {
                    return null;
                }
                asyncResult.AsyncWaitHandle.WaitOne();
                return this.guiControl.EndInvoke(asyncResult);
            }
            if (args2 != null)
            {
                args2.IsGuiThread = false;
            }
            return del.DynamicInvoke(args);
        }

        protected virtual void InvokeEventHandler(Delegate eventHandler, object sender, EventArgs e)
        {
            this.InvokeEventHandler(true, eventHandler, sender, e);
        }

        protected virtual void InvokeEventHandler(bool preferGuiThread, Delegate eventHandler, object sender, EventArgs e)
        {
            bool flag = (e is FTPCancelableEventArgs) && ((FTPCancelableEventArgs) e).CanBeCancelled;
            this.InvokeDelegate(preferGuiThread, !flag, eventHandler, new object[] { sender, e });
        }

        [MethodIdentifier(MethodIdentifier.InvokeFTPCommand, typeof(string), typeof(string[]))]
        public virtual FTPReply InvokeFTPCommand(string command, params string[] validCodes)
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                this.ftpClient.Quote(command, validCodes);
                return this.LastValidReply;
            }
        }

        [MethodIdentifier(MethodIdentifier.InvokeSiteCommand, typeof(string), typeof(string[]))]
        public virtual FTPReply InvokeSiteCommand(string command, params string[] arguments)
        {
            this.CheckFTPType(true);
            StringBuilder builder = new StringBuilder(command);
            foreach (string str in arguments)
            {
                builder.Append(" ");
                builder.Append(str);
            }
            lock (this.clientLock)
            {
                this.ftpClient.Site(builder.ToString());
                return this.LastValidReply;
            }
        }

        public void LinkComponent(IFTPComponent component)
        {
        }

        [MethodIdentifier(MethodIdentifier.Login)]
        public virtual void Login()
        {
            this.CheckFTPType(true);
            this.OnLoggingIn(this.loginUserName, this.loginPassword, false);
            bool hasLoggedIn = false;
            lock (this.clientLock)
            {
                try
                {
                    this.ftpClient.Login(this.loginUserName, this.loginPassword);
                    hasLoggedIn = true;
                }
                finally
                {
                    this.OnLoggedIn(this.loginUserName, this.loginPassword, hasLoggedIn);
                }
            }
        }

        private void OnActivePortRangeChanged(object sender, PropertyChangedEventArgs e)
        {
            this.OnPropertyChanged("ActivePortRange." + e.PropertyName);
        }

        protected virtual void OnBytesTransferred(string remoteFile, long byteCount, long resumeOffset)
        {
            this.RaiseBytesTransferred(new BytesTransferredEventArgs(this.remoteDir, remoteFile, byteCount, resumeOffset));
        }

        protected virtual void OnChangedLocalDirectory(string oldDirectory, string newDirectory, bool wasCancelled)
        {
            this.RaiseLocalDirectoryChanged(new FTPDirectoryEventArgs(oldDirectory, newDirectory, null));
        }

        protected virtual void OnChangedServerDirectory(string oldDirectory, string newDirectory, bool wasCancelled, Exception ex)
        {
            if (this.areEventsEnabled && (this.ServerDirectoryChanged != null))
            {
                this.RaiseServerDirectoryChanged(new FTPDirectoryEventArgs(oldDirectory, newDirectory, ex));
            }
            if (this.areEventsEnabled && (this.DirectoryChanged != null))
            {
                this.RaiseServerDirectoryChanged(new FTPDirectoryEventArgs(oldDirectory, newDirectory, ex));
            }
        }

        protected virtual bool OnChangingLocalDirectory(string oldDirectory, string newDirectory)
        {
            if (this.areEventsEnabled && (this.LocalDirectoryChanging != null))
            {
                FTPDirectoryEventArgs e = new FTPDirectoryEventArgs(oldDirectory, newDirectory, false, null);
                this.RaiseLocalDirectoryChanging(e);
                return !e.Cancel;
            }
            return true;
        }

        protected virtual bool OnChangingServerDirectory(string oldDirectory, string newDirectory)
        {
            if (!this.areEventsEnabled || ((this.ServerDirectoryChanging == null) && (this.DirectoryChanging == null)))
            {
                return true;
            }
            if (!PathUtil.IsAbsolute(oldDirectory))
            {
                oldDirectory = PathUtil.Combine(this.ServerDirectory, oldDirectory);
            }
            if (!PathUtil.IsAbsolute(newDirectory))
            {
                newDirectory = PathUtil.Combine(this.ServerDirectory, newDirectory);
            }
            if (this.ServerDirectoryChanging != null)
            {
                FTPDirectoryEventArgs args = new FTPDirectoryEventArgs(oldDirectory, newDirectory, false, null);
                this.RaiseServerDirectoryChanging(args);
                return !args.Cancel;
            }
            FTPDirectoryEventArgs e = new FTPDirectoryEventArgs(oldDirectory, newDirectory, false, null);
            this.RaiseServerDirectoryChanging(e);
            return !e.Cancel;
        }

        protected virtual void OnClosed()
        {
            this.RaiseClosed(new FTPConnectionEventArgs(this.ServerAddress, this.ServerPort, false));
        }

        protected virtual void OnClosing()
        {
            this.RaiseClosing(new FTPConnectionEventArgs(this.ServerAddress, this.ServerPort, true));
        }

        protected virtual void OnConnected(bool hasConnected)
        {
            this.RaiseConnected(new FTPConnectionEventArgs(this.ServerAddress, this.ServerPort, hasConnected));
        }

        protected virtual void OnConnecting()
        {
            this.RaiseConnecting(new FTPConnectionEventArgs(this.ServerAddress, this.ServerPort, false));
        }

        protected virtual void OnCreatedDirectory(string dir, bool cancelled, Exception ex)
        {
            if (this.areEventsEnabled && (this.CreatedDirectory != null))
            {
                if (!PathUtil.IsAbsolute(dir))
                {
                    dir = PathUtil.Combine(this.ServerDirectory, dir);
                }
                this.RaiseCreatedDirectory(new FTPDirectoryEventArgs(null, dir, cancelled, ex));
            }
        }

        protected virtual bool OnCreatingDirectory(string dir)
        {
            if (!this.areEventsEnabled || (this.CreatingDirectory == null))
            {
                return true;
            }
            if (!PathUtil.IsAbsolute(dir))
            {
                dir = PathUtil.Combine(this.ServerDirectory, dir);
            }
            FTPDirectoryEventArgs e = new FTPDirectoryEventArgs(null, dir, null);
            this.RaiseCreatingDirectory(e);
            return !e.Cancel;
        }

        protected virtual void OnDeleted(string remoteFile, bool cancelled, Exception ex)
        {
            this.RaiseDeleted(new FTPFileTransferEventArgs(false, remoteFile, this.ServerDirectory, -1L, false, cancelled, ex));
        }

        protected virtual void OnDeletedDirectory(string dir, bool cancelled, Exception ex)
        {
            if (this.areEventsEnabled && (this.DeletedDirectory != null))
            {
                if (!PathUtil.IsAbsolute(dir))
                {
                    dir = PathUtil.Combine(this.ServerDirectory, dir);
                }
                this.RaiseDeletedDirectory(new FTPDirectoryEventArgs(null, dir, cancelled, ex));
            }
        }

        protected bool OnDeleting(string remoteFile)
        {
            if (this.areEventsEnabled && (this.Deleting != null))
            {
                FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, remoteFile, this.ServerDirectory, -1L, false, false, null);
                this.RaiseDeleting(e);
                return !e.Cancel;
            }
            return true;
        }

        protected virtual bool OnDeletingDirectory(string dir)
        {
            if (!this.areEventsEnabled || (this.DeletingDirectory == null))
            {
                return true;
            }
            if (!PathUtil.IsAbsolute(dir))
            {
                dir = PathUtil.Combine(this.ServerDirectory, dir);
            }
            FTPDirectoryEventArgs e = new FTPDirectoryEventArgs(null, dir, null);
            this.RaiseDeletingDirectory(e);
            return !e.Cancel;
        }

        private void OnDirectoryEmptyMessagesChanged(object sender, PropertyChangedEventArgs e)
        {
            this.OnPropertyChanged("DirectoryEmptyMessages");
        }

        protected virtual void OnDirectoryListed(string dir, FTPFile[] files)
        {
            if (this.areEventsEnabled && (this.DirectoryListed != null))
            {
                if (!PathUtil.IsAbsolute(dir))
                {
                    dir = PathUtil.Combine(this.ServerDirectory, dir);
                }
                this.RaiseDirectoryListed(new FTPDirectoryListEventArgs(dir, files));
            }
        }

        protected virtual void OnDirectoryListing(string dir)
        {
            if (this.areEventsEnabled && (this.DirectoryListing != null))
            {
                if (!PathUtil.IsAbsolute(dir))
                {
                    dir = PathUtil.Combine(this.ServerDirectory, dir);
                }
                this.RaiseDirectoryListing(new FTPDirectoryListEventArgs(dir));
            }
        }

        protected virtual void OnDownloaded(byte[] bytes, string remoteFile, Exception ex)
        {
            this.RaiseDownloaded(new FTPFileTransferEventArgs(false, bytes, remoteFile, this.ServerDirectory, this.currentFileSize, false, this.lastTransferCancel, ex));
        }

        protected virtual void OnDownloaded(Stream destStream, string remoteFile, Exception ex)
        {
            this.RaiseDownloaded(new FTPFileTransferEventArgs(false, destStream, remoteFile, this.ServerDirectory, this.currentFileSize, false, this.lastTransferCancel, ex));
        }

        protected virtual void OnDownloaded(string localPath, string remoteFile, Exception ex)
        {
            this.RaiseDownloaded(new FTPFileTransferEventArgs(false, localPath, remoteFile, this.ServerDirectory, this.currentFileSize, false, this.lastTransferCancel, ex));
        }

        protected bool OnDownloading(string remoteFile)
        {
            if (this.areEventsEnabled && ((this.Downloading != null) || (this.Downloaded != null)))
            {
                this.currentFileSize = this.GetSize(remoteFile, false);
            }
            if (this.areEventsEnabled && (this.Downloading != null))
            {
                FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, remoteFile, this.ServerDirectory, this.currentFileSize, false, false, null);
                this.RaiseDownloading(e);
                this.lastTransferCancel = e.Cancel;
                return !e.Cancel;
            }
            return true;
        }

        protected bool OnDownloading(Stream destStream, string remoteFile)
        {
            if (this.areEventsEnabled && ((this.Downloading != null) || (this.Downloaded != null)))
            {
                this.currentFileSize = this.GetSize(remoteFile, false);
            }
            if (this.areEventsEnabled && (this.Downloading != null))
            {
                FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, destStream, remoteFile, this.ServerDirectory, this.currentFileSize, false, false, null);
                this.RaiseDownloading(e);
                this.lastTransferCancel = e.Cancel;
                return !e.Cancel;
            }
            return true;
        }

        protected bool OnDownloading(ref string localPath, string remoteFile)
        {
            if (this.areEventsEnabled && ((this.Downloading != null) || (this.Downloaded != null)))
            {
                this.currentFileSize = this.GetSize(remoteFile, false);
            }
            if (this.areEventsEnabled && (this.Downloading != null))
            {
                FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, localPath, remoteFile, this.ServerDirectory, this.currentFileSize, false, false, null);
                this.RaiseDownloading(e);
                localPath = e.LocalPath;
                this.lastTransferCancel = e.Cancel;
                return !e.Cancel;
            }
            return true;
        }

        private void OnFileNotFoundMessagesChanged(object sender, PropertyChangedEventArgs e)
        {
            this.OnPropertyChanged("FileNotFoundMessagesChanged");
        }

        protected virtual void OnLoggedIn(string userName, string password, bool hasLoggedIn)
        {
            this.RaiseLoggedIn(new FTPLogInEventArgs(userName, password, hasLoggedIn));
        }

        protected virtual void OnLoggingIn(string userName, string password, bool hasLoggedIn)
        {
            this.RaiseLoggingIn(new FTPLogInEventArgs(userName, password, hasLoggedIn));
        }

        protected virtual void OnPropertyChanged(string propertyName)
        {
            if (this.PropertyChanged != null)
            {
                this.PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        protected virtual void OnRenamed(string from, string to, bool cancelled, Exception ex)
        {
            if (this.areEventsEnabled && (this.RenamedFile != null))
            {
                if (!PathUtil.IsAbsolute(from))
                {
                    from = PathUtil.Combine(this.ServerDirectory, from);
                }
                if (!PathUtil.IsAbsolute(to))
                {
                    to = PathUtil.Combine(this.ServerDirectory, to);
                }
                this.RaiseRenamedFile(new FTPFileRenameEventArgs(false, from, to, cancelled, ex));
            }
        }

        protected virtual bool OnRenaming(string from, string to)
        {
            if (!this.areEventsEnabled || (this.RenamingFile == null))
            {
                return true;
            }
            if (!PathUtil.IsAbsolute(from))
            {
                from = PathUtil.Combine(this.ServerDirectory, from);
            }
            if (!PathUtil.IsAbsolute(to))
            {
                to = PathUtil.Combine(this.ServerDirectory, to);
            }
            FTPFileRenameEventArgs e = new FTPFileRenameEventArgs(true, from, to, false, null);
            this.RaiseRenamingFile(e);
            return !e.Cancel;
        }

        private void OnTransferCompleteMessagesChanged(object sender, PropertyChangedEventArgs e)
        {
            this.OnPropertyChanged("FileNotFoundMessages");
        }

        protected virtual void OnUploaded(string localPath, string remoteFile, bool append, Exception ex)
        {
            if (this.areEventsEnabled && (this.Uploaded != null))
            {
                if (!Path.IsPathRooted(localPath))
                {
                    localPath = Path.Combine(this.localDir, localPath);
                }
                long fileSize = 0L;
                try
                {
                    fileSize = new FileInfo(localPath).Length;
                }
                catch (Exception exception)
                {
                    string message = "Failed to open file '" + localPath + "'";
                    this.log.Error(message, exception);
                    throw new FTPException(message);
                }
                this.RaiseUploaded(new FTPFileTransferEventArgs(false, localPath, remoteFile, this.ServerDirectory, fileSize, append, this.lastTransferCancel, ex));
            }
        }

        protected virtual void OnUploaded(byte[] bytes, string remoteFile, bool append, Exception ex)
        {
            this.RaiseUploaded(new FTPFileTransferEventArgs(false, bytes, remoteFile, this.ServerDirectory, (long) bytes.Length, append, this.lastTransferCancel, ex));
        }

        protected virtual void OnUploaded(Stream srcStream, long size, string remoteFile, bool append, Exception ex)
        {
            this.RaiseUploaded(new FTPFileTransferEventArgs(false, srcStream, remoteFile, this.ServerDirectory, size, append, this.lastTransferCancel, ex));
        }

        protected bool OnUploading(byte[] bytes, ref string remoteFile, bool append)
        {
            if (this.areEventsEnabled && (this.Uploading != null))
            {
                FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, remoteFile, this.ServerDirectory, (long) bytes.Length, append, false, null);
                this.RaiseUploading(e);
                remoteFile = e.RemoteFile;
                this.lastTransferCancel = e.Cancel;
                return !e.Cancel;
            }
            return true;
        }

        protected bool OnUploading(Stream srcStream, ref string remoteFile, bool append)
        {
            if (this.areEventsEnabled && (this.Uploading != null))
            {
                FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, srcStream, remoteFile, this.ServerDirectory, srcStream.Length, append, false, null);
                this.RaiseUploading(e);
                remoteFile = e.RemoteFile;
                this.lastTransferCancel = e.Cancel;
                return !e.Cancel;
            }
            return true;
        }

        protected bool OnUploading(string localPath, ref string remoteFile, bool append)
        {
            if (!this.areEventsEnabled || (this.Uploading == null))
            {
                return true;
            }
            long fileSize = 0L;
            try
            {
                fileSize = new FileInfo(localPath).Length;
            }
            catch (Exception exception)
            {
                string message = "Failed to open file '" + localPath + "'";
                this.log.Error(message, exception);
                throw new FTPException(message);
            }
            FTPFileTransferEventArgs e = new FTPFileTransferEventArgs(true, localPath, remoteFile, this.ServerDirectory, fileSize, append, false, null);
            this.RaiseUploading(e);
            remoteFile = e.RemoteFile;
            this.lastTransferCancel = e.Cancel;
            return !e.Cancel;
        }

        protected virtual bool PerformAutoLogin()
        {
            bool hasLoggedIn = false;
            if (this.useAutoLogin && (this.loginUserName != null))
            {
                try
                {
                    this.OnLoggingIn(this.loginUserName, this.loginPassword, false);
                    this.ftpClient.User(this.loginUserName);
                    if ((this.loginPassword != null) && (this.ftpClient.LastValidReply.ReplyCode != "230"))
                    {
                        this.ftpClient.Password(this.loginPassword);
                    }
                    if ((this.accountInfoStr != null) && (this.ftpClient.LastValidReply.ReplyCode == "332"))
                    {
                        this.ftpClient.Account(this.accountInfoStr);
                    }
                    hasLoggedIn = true;
                    this.log.Debug("Successfully logged in");
                }
                finally
                {
                    this.OnLoggedIn(this.loginUserName, this.loginPassword, hasLoggedIn);
                }
            }
            return hasLoggedIn;
        }

        protected virtual void PostLogin()
        {
            this.ActiveClient.TransferType = this.fileTransferType;
            if ((this.remoteDir != null) && (this.remoteDir.Trim().Length > 0))
            {
                try
                {
                    this.ChangeWorkingDirectory(this.remoteDir);
                }
                catch (Exception exception)
                {
                    this.log.Error("Failed to change working directory to '" + this.remoteDir + "': " + exception.Message);
                    this.remoteDir = this.ActiveClient.Pwd();
                    this.log.Warn("Set working directory to '" + this.remoteDir + "'");
                }
            }
            else
            {
                this.remoteDir = this.ActiveClient.Pwd();
                this.OnChangingServerDirectory(null, this.remoteDir);
                this.OnChangedServerDirectory(null, this.remoteDir, false, null);
            }
        }

        protected internal void RaiseBytesTransferred(BytesTransferredEventArgs e)
        {
            if (this.areEventsEnabled && (this.BytesTransferred != null))
            {
                this.InvokeEventHandler(this.BytesTransferred, this, e);
            }
        }

        protected internal void RaiseClosed(FTPConnectionEventArgs e)
        {
            if (this.areEventsEnabled && (this.Closed != null))
            {
                this.InvokeEventHandler(this.Closed, this, e);
            }
        }

        protected internal void RaiseClosing(FTPConnectionEventArgs e)
        {
            if (this.areEventsEnabled && (this.Closing != null))
            {
                this.InvokeEventHandler(this.Closing, this, e);
            }
        }

        protected internal void RaiseCommandSent(FTPMessageEventArgs e)
        {
            if (this.areEventsEnabled && (this.CommandSent != null))
            {
                this.InvokeEventHandler(this.CommandSent, this, e);
            }
        }

        protected internal void RaiseConnected(FTPConnectionEventArgs e)
        {
            if (this.areEventsEnabled && (this.Connected != null))
            {
                this.InvokeEventHandler(this.Connected, this, e);
            }
        }

        protected internal void RaiseConnecting(FTPConnectionEventArgs e)
        {
            if (this.areEventsEnabled && (this.Connecting != null))
            {
                this.InvokeEventHandler(this.Connecting, this, e);
            }
        }

        protected internal void RaiseCreatedDirectory(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.CreatedDirectory != null))
            {
                this.InvokeEventHandler(this.CreatedDirectory, this, e);
            }
        }

        protected internal void RaiseCreatingDirectory(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.CreatingDirectory != null))
            {
                this.InvokeEventHandler(this.CreatingDirectory, this, e);
            }
        }

        protected internal void RaiseDeleted(FTPFileTransferEventArgs e)
        {
            if (this.areEventsEnabled && (this.Deleted != null))
            {
                this.InvokeEventHandler(this.Deleted, this, e);
            }
        }

        protected internal void RaiseDeletedDirectory(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.DeletedDirectory != null))
            {
                this.InvokeEventHandler(this.DeletedDirectory, this, e);
            }
        }

        protected internal void RaiseDeleting(FTPFileTransferEventArgs e)
        {
            if (this.areEventsEnabled && (this.Deleting != null))
            {
                this.InvokeEventHandler(this.Deleting, this, e);
            }
        }

        protected internal void RaiseDeletingDirectory(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.DeletingDirectory != null))
            {
                this.InvokeEventHandler(this.DeletingDirectory, this, e);
            }
        }

        protected internal void RaiseDirectoryChanged(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.DirectoryChanged != null))
            {
                this.InvokeEventHandler(this.DirectoryChanged, this, e);
            }
        }

        protected internal void RaiseDirectoryChanging(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.DirectoryChanging != null))
            {
                this.InvokeEventHandler(this.DirectoryChanging, this, e);
            }
        }

        protected internal void RaiseDirectoryListed(FTPDirectoryListEventArgs e)
        {
            if (this.areEventsEnabled && (this.DirectoryListed != null))
            {
                this.InvokeEventHandler(this.DirectoryListed, this, e);
            }
        }

        protected internal void RaiseDirectoryListing(FTPDirectoryListEventArgs e)
        {
            if (this.areEventsEnabled && (this.DirectoryListing != null))
            {
                this.InvokeEventHandler(this.DirectoryListing, this, e);
            }
        }

        protected internal void RaiseDownloaded(FTPFileTransferEventArgs e)
        {
            if (this.areEventsEnabled && (this.Downloaded != null))
            {
                this.InvokeEventHandler(this.Downloaded, this, e);
            }
        }

        protected internal void RaiseDownloading(FTPFileTransferEventArgs e)
        {
            if (this.areEventsEnabled && (this.Downloading != null))
            {
                this.InvokeEventHandler(this.Downloading, this, e);
            }
        }

        protected internal void RaiseLocalDirectoryChanged(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.LocalDirectoryChanged != null))
            {
                this.InvokeEventHandler(this.LocalDirectoryChanged, this, e);
            }
        }

        protected internal void RaiseLocalDirectoryChanging(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.LocalDirectoryChanging != null))
            {
                this.InvokeEventHandler(this.LocalDirectoryChanging, this, e);
            }
        }

        protected internal void RaiseLoggedIn(FTPLogInEventArgs e)
        {
            if (this.areEventsEnabled && (this.LoggedIn != null))
            {
                this.InvokeEventHandler(this.LoggedIn, this, e);
            }
        }

        protected internal void RaiseLoggingIn(FTPLogInEventArgs e)
        {
            if (this.areEventsEnabled && (this.LoggingIn != null))
            {
                this.InvokeEventHandler(this.LoggingIn, this, e);
            }
        }

        protected internal void RaisePropertyChanged(PropertyChangedEventArgs e)
        {
            if (this.areEventsEnabled && (this.PropertyChanged != null))
            {
                this.InvokeEventHandler(this.PropertyChanged, this, e);
            }
        }

        protected internal void RaiseRenamedFile(FTPFileRenameEventArgs e)
        {
            if (this.areEventsEnabled && (this.RenamedFile != null))
            {
                this.InvokeEventHandler(this.RenamedFile, this, e);
            }
        }

        protected internal void RaiseRenamingFile(FTPFileRenameEventArgs e)
        {
            if (this.areEventsEnabled && (this.RenamingFile != null))
            {
                this.InvokeEventHandler(this.RenamingFile, this, e);
            }
        }

        protected internal void RaiseReplyReceived(FTPMessageEventArgs e)
        {
            if (this.areEventsEnabled && (this.ReplyReceived != null))
            {
                this.InvokeEventHandler(this.ReplyReceived, this, e);
            }
        }

        protected internal void RaiseServerDirectoryChanged(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.ServerDirectoryChanged != null))
            {
                this.InvokeEventHandler(this.ServerDirectoryChanged, this, e);
            }
        }

        protected internal void RaiseServerDirectoryChanging(FTPDirectoryEventArgs e)
        {
            if (this.areEventsEnabled && (this.ServerDirectoryChanging != null))
            {
                this.InvokeEventHandler(this.ServerDirectoryChanging, this, e);
            }
        }

        protected internal void RaiseUploaded(FTPFileTransferEventArgs e)
        {
            if (this.areEventsEnabled && (this.Uploaded != null))
            {
                this.InvokeEventHandler(this.Uploaded, this, e);
            }
        }

        protected internal void RaiseUploading(FTPFileTransferEventArgs e)
        {
            if (this.areEventsEnabled && (this.Uploading != null))
            {
                this.InvokeEventHandler(this.Uploading, this, e);
            }
        }

        protected string RelativePathToAbsolute(string absolutePath, string relativePath)
        {
            this.log.Debug("Combining absolute path '" + absolutePath + "' with relative path '" + relativePath + "'");
            if (Path.IsPathRooted(relativePath))
            {
                return relativePath;
            }
            if (Path.GetFileName(relativePath) == relativePath)
            {
                return Path.Combine(absolutePath, relativePath);
            }
            string[] strArray2 = relativePath.Split(@"\".ToCharArray());
            string path = absolutePath;
            bool flag = false;
            for (int i = 0; i < strArray2.Length; i++)
            {
                if ((strArray2[i].Length != 0) && (strArray2[i] != "."))
                {
                    if (strArray2[i] == "..")
                    {
                        if (flag)
                        {
                            throw new IOException("Cannot embed '..' in middle of path");
                        }
                        path = new DirectoryInfo(path).Parent.FullName;
                    }
                    else
                    {
                        flag = true;
                        if ((path.Length > 0) && (path[path.Length - 1] != '\\'))
                        {
                            path = path + @"\";
                        }
                        path = path + strArray2[i];
                    }
                }
            }
            return path;
        }

        [MethodIdentifier(MethodIdentifier.RenameFile, typeof(string), typeof(string))]
        public virtual bool RenameFile(string from, string to)
        {
            lock (this.clientLock)
            {
                bool cancelled = false;
                Exception ex = null;
                try
                {
                    if (this.OnRenaming(from, to))
                    {
                        this.ActiveClient.Rename(from, to);
                    }
                    else
                    {
                        cancelled = true;
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnRenamed(from, to, cancelled, ex);
                }
                return !cancelled;
            }
        }

        [MethodIdentifier(MethodIdentifier.ResumeTransfer)]
        public virtual void ResumeTransfer()
        {
            this.log.Info("Resuming transfer");
            this.ActiveClient.Resume();
        }

        private object RunDelegate(RunDelegateArgs delArgs)
        {
            object obj2;
            try
            {
                obj2 = delArgs.Delegate.DynamicInvoke(delArgs.Arguments);
            }
            catch (Exception exception)
            {
                this.log.Error(exception);
                obj2 = null;
            }
            finally
            {
                invokeSemaphore.Release();
            }
            return obj2;
        }

        public virtual void SendAccountInfo(string accountInfo)
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                this.ftpClient.Account(accountInfo);
                if (this.ftpClient.LastValidReply.ReplyCode == "230")
                {
                    this.PostLogin();
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.SendPassword, typeof(string))]
        public virtual void SendPassword(string loginPassword)
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                this.ftpClient.Password(loginPassword);
                if (this.ftpClient.LastValidReply.ReplyCode == "230")
                {
                    this.PostLogin();
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.SendUserName, typeof(string))]
        public virtual void SendUserName(string user)
        {
            this.CheckFTPType(true);
            lock (this.clientLock)
            {
                this.ftpClient.User(user);
                if (this.ftpClient.LastValidReply.ReplyCode == "230")
                {
                    this.PostLogin();
                }
            }
        }

        internal void SetIsTransferring(bool isTransferring)
        {
            this.isTransferringData = isTransferring;
        }

        public virtual void SetLastWriteTime(string remoteFile, DateTime lastWriteTime)
        {
            lock (this.clientLock)
            {
                this.ActiveClient.SetModTime(remoteFile, lastWriteTime);
            }
        }

        [MethodIdentifier(MethodIdentifier.UploadByteArray, typeof(byte[]), typeof(string))]
        public virtual void UploadByteArray(byte[] bytes, string remoteFile)
        {
            this.UploadByteArray(bytes, remoteFile, false);
        }

        [MethodIdentifier(MethodIdentifier.UploadByteArray, typeof(byte[]), typeof(string), typeof(bool))]
        public virtual void UploadByteArray(byte[] bytes, string remoteFile, bool append)
        {
            lock (this.clientLock)
            {
                Exception ex = null;
                try
                {
                    this.lastTransferCancel = false;
                    if (this.OnUploading(bytes, ref remoteFile, append))
                    {
                        try
                        {
                            this.isTransferringData = true;
                            this.ActiveClient.Put(bytes, remoteFile, append);
                        }
                        finally
                        {
                            this.isTransferringData = false;
                        }
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnUploaded(bytes, remoteFile, append, ex);
                }
            }
        }

        public virtual void UploadFile(string localPath, string remoteFile)
        {
            this.UploadFile(localPath, remoteFile, false);
        }

        public virtual void UploadFile(string localPath, string remoteFile, bool append)
        {
            this.log.Debug(string.Concat(new object[] { "UploadFile(", localPath, ",", remoteFile, ",", append, ")" }));
            lock (this.clientLock)
            {
                Exception ex = null;
                string str = localPath;
                if (this.localDir != null)
                {
                    str = this.RelativePathToAbsolute(this.localDir, localPath);
                }
                else
                {
                    str = this.RelativePathToAbsolute(Directory.GetCurrentDirectory(), this.localDir);
                }
                try
                {
                    this.lastTransferCancel = false;
                    if (this.OnUploading(str, ref remoteFile, append))
                    {
                        try
                        {
                            this.isTransferringData = true;
                            this.ActiveClient.Put(str, remoteFile, append);
                        }
                        finally
                        {
                            this.isTransferringData = false;
                        }
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnUploaded(str, remoteFile, append, ex);
                }
            }
        }

        [MethodIdentifier(MethodIdentifier.UploadStream, typeof(Stream), typeof(string))]
        public virtual void UploadStream(Stream srcStream, string remoteFile)
        {
            this.UploadStream(srcStream, remoteFile, false);
        }

        [MethodIdentifier(MethodIdentifier.UploadStream, typeof(Stream), typeof(string), typeof(bool))]
        public virtual void UploadStream(Stream srcStream, string remoteFile, bool append)
        {
            lock (this.clientLock)
            {
                long size = 0L;
                Exception ex = null;
                try
                {
                    this.lastTransferCancel = false;
                    if (this.OnUploading(srcStream, ref remoteFile, append))
                    {
                        try
                        {
                            this.isTransferringData = true;
                            size = this.ActiveClient.Put(srcStream, remoteFile, append);
                        }
                        finally
                        {
                            this.isTransferringData = false;
                        }
                    }
                }
                catch (Exception exception2)
                {
                    ex = exception2;
                    throw;
                }
                finally
                {
                    this.OnUploaded(srcStream, size, remoteFile, append, ex);
                }
            }
        }

        [Description("Account information string used in FTP/FTPS."), Category("FTP/FTPS"), DefaultValue((string) null)]
        public virtual string AccountInfo
        {
            get
            {
                return this.accountInfoStr;
            }
            set
            {
                bool flag = this.AccountInfo != value;
                this.CheckConnection(false);
                this.accountInfoStr = value;
                if (flag)
                {
                    this.OnPropertyChanged("AccountInfo");
                }
            }
        }

        protected internal IFileTransferClient ActiveClient
        {
            get
            {
                return this.activeClient;
            }
            set
            {
                this.activeClient = value;
            }
        }

        [Category("FTP/FTPS"), TypeConverter(typeof(ExpandableObjectConverter)), Description("Specifies the range of ports to be used for data-channels in active mode."), DesignerSerializationVisibility(DesignerSerializationVisibility.Content)]
        public virtual PortRange ActivePortRange
        {
            get
            {
                return this.ftpClient.ActivePortRange;
            }
        }

        [DefaultValue(true), Description("Determines if the component will automatically log in upon connection."), Category("FTP/FTPS")]
        public bool AutoLogin
        {
            get
            {
                return this.useAutoLogin;
            }
            set
            {
                bool flag = this.AutoLogin != value;
                this.useAutoLogin = value;
                if (flag)
                {
                    this.OnPropertyChanged("AutoLogin");
                }
            }
        }

        [Description("Ensures that data-socket connections are made to the same IP address that the control socket is connected to."), DefaultValue(true), Category("FTP/FTPS")]
        public bool AutoPassiveIPSubstitution
        {
            get
            {
                return this.ftpClient.AutoPassiveIPSubstitution;
            }
            set
            {
                bool flag = this.AutoPassiveIPSubstitution != value;
                this.ftpClient.AutoPassiveIPSubstitution = value;
                if (flag)
                {
                    this.OnPropertyChanged("AutoPassiveIPSubstitution");
                }
            }
        }

        [Description("The build timestamp of the assembly."), Category("Version")]
        public string BuildTimestamp
        {
            get
            {
                return FTPClient.BuildTimestamp;
            }
        }

        [Category("Transfer"), Description("Determines if stream-based transfer-methods should close the stream once the transfer is completed."), DefaultValue(true)]
        public virtual bool CloseStreamsAfterTransfer
        {
            get
            {
                return this.ftpClient.CloseStreamsAfterTransfer;
            }
            set
            {
                bool flag = this.CloseStreamsAfterTransfer != value;
                this.ftpClient.CloseStreamsAfterTransfer = value;
                if (flag)
                {
                    this.OnPropertyChanged("CloseStreamsAfterTransfer");
                }
            }
        }

        [Description("The character-encoding to use for FTP control commands and file-names."), Category("FTP/FTPS"), DefaultValue((string) null)]
        public virtual Encoding CommandEncoding
        {
            get
            {
                return this.ftpClient.ControlEncoding;
            }
            set
            {
                bool flag = this.CommandEncoding != value;
                this.ftpClient.ControlEncoding = value;
                if (flag)
                {
                    this.OnPropertyChanged("CommandEncoding");
                }
            }
        }

        [Category("FTP/FTPS"), Description("The connection-mode of data-channels.  Usually passive when FTP client is behind a firewall."), DefaultValue(2)]
        public FTPConnectMode ConnectMode
        {
            get
            {
                return this.ftpClient.ConnectMode;
            }
            set
            {
                lock (this.clientLock)
                {
                    bool flag = this.ConnectMode != value;
                    this.ftpClient.ConnectMode = value;
                    if (flag)
                    {
                        this.OnPropertyChanged("ConnectMode");
                    }
                }
            }
        }

        [DefaultValue((string) null), Category("FTP/FTPS/HTTP"), Description("The character-encoding to use for data transfers in ASCII mode only.")]
        public virtual Encoding DataEncoding
        {
            get
            {
                return this.ftpClient.DataEncoding;
            }
            set
            {
                bool flag = this.DataEncoding != value;
                this.ftpClient.DataEncoding = value;
                if (flag)
                {
                    this.OnPropertyChanged("DataEncoding");
                }
            }
        }

        [Category("Transfer"), DefaultValue(true), Description("Controls whether or not a file is deleted when a failure occurs while it is transferred.")]
        public virtual bool DeleteOnFailure
        {
            get
            {
                return this.ftpClient.DeleteOnFailure;
            }
            set
            {
                bool flag = this.DeleteOnFailure != value;
                this.ftpClient.DeleteOnFailure = value;
                if (flag)
                {
                    this.OnPropertyChanged("DeleteOnFailure");
                }
            }
        }

        [Category("FTP/FTPS"), Description("Holds fragments of server messages that indicate a directory is empty.")]
        public DirectoryEmptyStrings DirectoryEmptyMessages
        {
            get
            {
                return this.ftpClient.DirectoryEmptyMessages;
            }
        }

        [Browsable(false), DefaultValue(true)]
        public bool EventsEnabled
        {
            get
            {
                return this.areEventsEnabled;
            }
            set
            {
                bool flag = this.EventsEnabled != value;
                this.areEventsEnabled = value;
                if (flag)
                {
                    this.OnPropertyChanged("EventsEnabled");
                }
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), Browsable(false)]
        public FTPFileFactory FileInfoParser
        {
            get
            {
                return this.ftpClient.FTPFileFactory;
            }
            set
            {
                bool flag = this.FileInfoParser != value;
                this.ftpClient.FTPFileFactory = value;
                if (flag)
                {
                    this.OnPropertyChanged("FileInfoParser");
                }
            }
        }

        [Category("FTP/FTPS"), Description("Holds fragments of server messages that indicate a file was not found.")]
        public FileNotFoundStrings FileNotFoundMessages
        {
            get
            {
                return this.ftpClient.FileNotFoundMessages;
            }
        }

        [Obsolete("Use CommandEncoding"), EditorBrowsable(EditorBrowsableState.Never), Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public Encoding FilePathEncoding
        {
            get
            {
                return this.CommandEncoding;
            }
            set
            {
                this.CommandEncoding = value;
            }
        }

        internal int InstanceNumber
        {
            get
            {
                return this.instanceNumber;
            }
        }

        [Browsable(false)]
        public bool IsConnected
        {
            get
            {
                return this.ActiveClient.IsConnected;
            }
        }

        [Browsable(false)]
        public virtual bool IsTransferring
        {
            get
            {
                return this.isTransferringData;
            }
        }

        [Browsable(false)]
        public bool LastTransferCancelled
        {
            get
            {
                return this.lastTransferCancel;
            }
        }

        [Browsable(false)]
        public FTPReply LastValidReply
        {
            get
            {
                return this.ftpClient.LastValidReply;
            }
        }

        [DefaultValue((string) null), PropertyOrder(6), Category("Connection"), Description("Working directory on the local file-system into which files are downloaded.")]
        public string LocalDirectory
        {
            get
            {
                return this.localDir;
            }
            set
            {
                if (this.localDir != value)
                {
                    string localDir = this.localDir;
                    if (base.DesignMode)
                    {
                        this.localDir = value;
                    }
                    else
                    {
                        if ((value != null) && !Directory.Exists(value))
                        {
                            this.log.Error("Directory {0} does not exist.  Leaving LocalDirectory unchanged.", null, new object[] { value });
                            return;
                        }
                        if (this.OnChangingLocalDirectory(this.localDir, value))
                        {
                            if (!Path.IsPathRooted(value))
                            {
                                throw new IOException("The specified path '" + value + "' is not absolute.");
                            }
                            this.localDir = value;
                            this.log.Debug("Set LocalDirectory='" + value + "'");
                            this.OnChangedLocalDirectory(localDir, this.localDir, false);
                        }
                        else
                        {
                            this.OnChangedLocalDirectory(localDir, this.localDir, true);
                        }
                    }
                    this.OnPropertyChanged("LocalDirectory");
                }
            }
        }

        [Category("Logging"), DefaultValue((string) null), Description("Name of file to which logs will be written.")]
        public static string LogFile
        {
            get
            {
                return Logger.PrimaryLogFile;
            }
            set
            {
                Logger.PrimaryLogFile = value;
            }
        }

        [DefaultValue(3), Category("Logging"), Description("Level of logging to be written '")]
        public static EnterpriseDT.Util.Debug.LogLevel LogLevel
        {
            get
            {
                return Logger.CurrentLevel.GetLevel();
            }
            set
            {
                Logger.CurrentLevel = Level.GetLevel(value);
            }
        }

        [DefaultValue(false), Category("Logging"), Description("Determines whether or not logs will be written to the console.")]
        public static bool LogToConsole
        {
            get
            {
                return Logger.LogToConsole;
            }
            set
            {
                Logger.LogToConsole = value;
            }
        }

        [Description("Determines whether or not logs will be written using .NET's trace."), DefaultValue(false), Category("Logging")]
        public static bool LogToTrace
        {
            get
            {
                return Logger.LogToTrace;
            }
            set
            {
                Logger.LogToTrace = value;
            }
        }

        [DefaultValue((string) null), Description("Name of the connection."), PropertyOrder(14), Category("Connection")]
        public string Name
        {
            get
            {
                return this.name;
            }
            set
            {
                bool flag = this.name != value;
                this.name = value;
                if (flag)
                {
                    this.OnPropertyChanged("Name");
                }
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Visible), Browsable(false)]
        public Control ParentControl
        {
            get
            {
                return this.guiControl;
            }
            set
            {
                this.guiControl = value;
            }
        }

        [Description("The culture for parsing file listings."), DefaultValue(typeof(CultureInfo), ""), Category("FTP/FTPS")]
        public CultureInfo ParsingCulture
        {
            get
            {
                return this.ftpClient.ParsingCulture;
            }
            set
            {
                bool flag = this.ParsingCulture != value;
                this.ftpClient.ParsingCulture = value;
                if (flag)
                {
                    this.OnPropertyChanged("ParsingCulture");
                }
            }
        }

        [Description("Password of account on the server."), DefaultValue((string) null), PropertyOrder(4), Category("Connection")]
        public virtual string Password
        {
            get
            {
                return this.loginPassword;
            }
            set
            {
                bool flag = this.Password != value;
                this.loginPassword = value;
                if (flag)
                {
                    this.OnPropertyChanged("Password");
                }
            }
        }

        [PropertyOrder(0), Category("Connection"), Description("File transfer protocol to use."), DefaultValue(0)]
        public virtual FileTransferProtocol Protocol
        {
            get
            {
                return FileTransferProtocol.FTP;
            }
            set
            {
                this.CheckConnection(false);
                if (value != FileTransferProtocol.FTP)
                {
                    throw new FTPException("FTPConnection only supports standard FTP.  " + value + " is supported in SecureFTPConnection.\nSecureFTPConnection is available in edtFTPnet/PRO (www.enterprisedt.com/products/edtftpnetpro).");
                }
            }
        }

        [Description("IP address of the client as the server sees it."), DefaultValue(""), Category("FTP/FTPS")]
        public string PublicIPAddress
        {
            get
            {
                if (this.ftpClient.ActiveIPAddress == null)
                {
                    return "";
                }
                return this.ftpClient.ActiveIPAddress.ToString();
            }
            set
            {
                bool flag = this.PublicIPAddress != value;
                if ((value == null) || (value == ""))
                {
                    this.ftpClient.ActiveIPAddress = null;
                }
                else
                {
                    try
                    {
                        this.ftpClient.ActiveIPAddress = IPAddress.Parse(value);
                    }
                    catch (FormatException)
                    {
                        this.ftpClient.ActiveIPAddress = null;
                    }
                }
                if (flag)
                {
                    this.OnPropertyChanged("PublicIPAddress");
                }
            }
        }

        [Description("The domain-name or IP address of the FTP server."), Category("Connection"), PropertyOrder(1), DefaultValue((string) null)]
        public virtual string ServerAddress
        {
            get
            {
                return this.ftpClient.RemoteHost;
            }
            set
            {
                bool flag = this.ServerAddress != value;
                this.ftpClient.RemoteHost = value;
                if (flag)
                {
                    this.OnPropertyChanged("ServerAddress");
                }
            }
        }

        [Category("Connection"), DefaultValue((string) null), PropertyOrder(5), Description("Current/initial working directory on server.")]
        public string ServerDirectory
        {
            get
            {
                return this.remoteDir;
            }
            set
            {
                lock (this.clientLock)
                {
                    bool flag = this.ServerDirectory != value;
                    if (this.IsConnected)
                    {
                        this.ChangeWorkingDirectory(value);
                    }
                    else
                    {
                        this.remoteDir = value;
                    }
                    if (flag)
                    {
                        this.OnPropertyChanged("ServerDirectory");
                    }
                }
            }
        }

        [DefaultValue(0x15), PropertyOrder(2), Description("Port on the server to which to connect the control-channel."), Category("Connection")]
        public virtual int ServerPort
        {
            get
            {
                return this.ftpClient.ControlPort;
            }
            set
            {
                bool flag = this.ServerPort != value;
                this.ftpClient.ControlPort = value;
                if (flag)
                {
                    this.OnPropertyChanged("ServerPort");
                }
            }
        }

        [Description("Include hidden files in operations that involve directory listings."), DefaultValue(false), Category("FTP/FTPS")]
        public virtual bool ShowHiddenFiles
        {
            get
            {
                return this.ftpClient.ShowHiddenFiles;
            }
            set
            {
                bool flag = this.ShowHiddenFiles != value;
                this.ftpClient.ShowHiddenFiles = value;
                if (flag)
                {
                    this.OnPropertyChanged("ShowHiddenFiles");
                }
            }
        }

        [Browsable(false)]
        public override ISite Site
        {
            get
            {
                return base.Site;
            }
            set
            {
                base.Site = value;
                this.guiControl = (Form) FTPComponentLinker.Find(value, typeof(Form));
                FTPComponentLinker.Link(value, this);
            }
        }

        [DefaultValue(false), Category("FTP/FTPS"), Description("Controls whether or not checking of return codes is strict.")]
        public bool StrictReturnCodes
        {
            get
            {
                return this.ftpClient.StrictReturnCodes;
            }
            set
            {
                bool flag = this.StrictReturnCodes != value;
                this.ftpClient.StrictReturnCodes = value;
                if (flag)
                {
                    this.OnPropertyChanged("StrictReturnCodes");
                }
            }
        }

        [Description("Used to synchronize the creation of passive data sockets."), DefaultValue(false), Category("FTP/FTPS")]
        public bool SynchronizePassiveConnections
        {
            get
            {
                return this.ftpClient.SynchronizePassiveConnections;
            }
            set
            {
                bool flag = this.SynchronizePassiveConnections != value;
                this.ftpClient.SynchronizePassiveConnections = value;
                if (flag)
                {
                    this.OnPropertyChanged("SynchronizePassiveConnections");
                }
            }
        }

        [Description("Time difference between server and client (relative to client)."), Category("FTP/FTPS"), DefaultValue(typeof(TimeSpan), "00:00:00")]
        public virtual TimeSpan TimeDifference
        {
            get
            {
                return this.ftpClient.TimeDifference;
            }
            set
            {
                bool flag = this.TimeDifference != value;
                this.ftpClient.TimeDifference = value;
                if (flag)
                {
                    this.OnPropertyChanged("TimeDifference");
                }
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), Browsable(false)]
        public bool TimeIncludesSeconds
        {
            get
            {
                return this.ftpClient.TimeIncludesSeconds;
            }
        }

        [Category("Transfer"), Description("TCP timeout (in milliseconds) on the underlying sockets (0 means none)."), DefaultValue(0x1d4c0)]
        public virtual int Timeout
        {
            get
            {
                return this.ftpClient.Timeout;
            }
            set
            {
                bool flag = this.Timeout != value;
                this.ftpClient.Timeout = value;
                if (flag)
                {
                    this.OnPropertyChanged("Timeout");
                }
            }
        }

        [Description("The size of the buffers used in writing to and reading from the data sockets."), Category("Transfer"), DefaultValue(0x1000)]
        public virtual int TransferBufferSize
        {
            get
            {
                return this.ftpClient.TransferBufferSize;
            }
            set
            {
                bool flag = this.TransferBufferSize != value;
                this.ftpClient.TransferBufferSize = value;
                if (flag)
                {
                    this.OnPropertyChanged("TransferBufferSize");
                }
            }
        }

        [Description("Holds fragments of server messages that indicate a transfer completed."), Category("FTP/FTPS")]
        public TransferCompleteStrings TransferCompleteMessages
        {
            get
            {
                return this.ftpClient.TransferCompleteMessages;
            }
        }

        [Description("The number of bytes transferred between each notification of the BytesTransferred event."), DefaultValue(0x1000), Category("Transfer")]
        public virtual long TransferNotifyInterval
        {
            get
            {
                return this.ftpClient.TransferNotifyInterval;
            }
            set
            {
                bool flag = this.TransferNotifyInterval != value;
                this.ftpClient.TransferNotifyInterval = value;
                if (flag)
                {
                    this.OnPropertyChanged("TransferNotifyInterval");
                }
            }
        }

        [DefaultValue(false), Category("Transfer"), Description("Controls if BytesTransferred event is triggered during directory listings.")]
        public virtual bool TransferNotifyListings
        {
            get
            {
                return this.ftpClient.TransferNotifyListings;
            }
            set
            {
                bool flag = this.TransferNotifyListings != value;
                this.ftpClient.TransferNotifyListings = value;
                if (flag)
                {
                    this.OnPropertyChanged("TransferNotifyListings");
                }
            }
        }

        [Description("The type of file transfer to use, i.e. BINARY or ASCII."), DefaultValue(2), Category("Transfer")]
        public virtual FTPTransferType TransferType
        {
            get
            {
                return this.fileTransferType;
            }
            set
            {
                lock (this.clientLock)
                {
                    bool flag = this.TransferType != value;
                    this.fileTransferType = value;
                    if (this.IsConnected)
                    {
                        this.ActiveClient.TransferType = value;
                    }
                    if (flag)
                    {
                        this.OnPropertyChanged("TransferType");
                    }
                }
            }
        }

        [DefaultValue(true), Browsable(false)]
        public bool UseGuiThreadIfAvailable
        {
            get
            {
                return this.useGuiThread;
            }
            set
            {
                bool flag = this.UseGuiThreadIfAvailable != value;
                this.useGuiThread = value;
                if (flag)
                {
                    this.OnPropertyChanged("UseGuiThreadIfAvailable");
                }
            }
        }

        [PropertyOrder(3), DefaultValue((string) null), Category("Connection"), Description("User-name of account on the server.")]
        public virtual string UserName
        {
            get
            {
                return this.loginUserName;
            }
            set
            {
                bool flag = this.UserName != value;
                this.CheckConnection(false);
                this.loginUserName = value;
                if (flag)
                {
                    this.OnPropertyChanged("UserName");
                }
            }
        }

        [Description("The assembly's version string."), Category("Version")]
        public string Version
        {
            get
            {
                int[] version = FTPClient.Version;
                return string.Concat(new object[] { version[0], ".", version[1], ".", version[2] });
            }
        }

        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), Browsable(false), Obsolete("Use ServerDirectory."), EditorBrowsable(EditorBrowsableState.Never)]
        public string WorkingDirectory
        {
            get
            {
                return this.ServerDirectory;
            }
            set
            {
                this.ServerDirectory = value;
            }
        }

        private class RunDelegateArgs
        {
            private object[] args;
            private System.Delegate del;

            public RunDelegateArgs(System.Delegate del, object[] args)
            {
                this.del = del;
                this.args = args;
            }

            public object[] Arguments
            {
                get
                {
                    return this.args;
                }
            }

            public System.Delegate Delegate
            {
                get
                {
                    return this.del;
                }
            }
        }

        private delegate object RunDelegateDelegate(FTPConnection.RunDelegateArgs delArgs);
    }
}

