namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Runtime.CompilerServices;
    using System.Text;

    public class FTPClient : IFileTransferClient
    {
        private IPAddress activeIPAddress;
        private PortRange activePortRange;
        private static string ASCII_CHAR = "A";
        private bool autoPassiveIPSubstitution;
        private static string BINARY_CHAR = "I";
        private static string buildTimestamp = "26-Oct-2009 13:44:34 EST";
        private bool cancelTransfer;
        private bool closeStreamsAfterTransfer;
        private FTPConnectMode connectMode;
        internal FTPControlSocket control;
        private Encoding controlEncoding;
        internal int controlPort;
        internal FTPDataSocket data;
        private Encoding dataEncoding;
        private const int DEFAULT_BUFFER_SIZE = 0x1000;
        private const int DEFAULT_MONITOR_INTERVAL = 0x1000;
        private const string DEFAULT_TIME_FORMAT = "yyyyMMddHHmmss";
        private bool deleteOnFailure;
        internal DirectoryEmptyStrings dirEmptyStrings;
        private EnterpriseDT.Net.Ftp.FTPFileFactory fileFactory;
        internal FileNotFoundStrings fileNotFoundStrings;
        internal FTPReply lastValidReply;
        private Logger log;
        private static string majorVersion = "2";
        private bool mdtmSupported;
        private static string middleVersion = "0";
        private static string minorVersion = "1";
        private string[] modtimeFormats;
        private long monitorInterval;
        protected int noOperationInterval;
        private CultureInfo parserCulture;
        internal string remoteHost;
        private bool resume;
        private long resumeMarker;
        private static int SHORT_TIMEOUT = 500;
        private bool showHiddenFiles;
        private bool sizeSupported;
        private bool strictReturnCodes;
        private bool synchronizePassiveConnections;
        protected BandwidthThrottler throttler;
        internal int timeout;
        private int transferBufferSize;
        internal TransferCompleteStrings transferCompleteStrings;
        private bool transferNotifyListings;
        private FTPTransferType transferType;
        private static int[] version;

        public event BytesTransferredHandler BytesTransferred;

        public event FTPMessageHandler CommandSent;

        public event FTPMessageHandler ReplyReceived;

        [Obsolete("Use TransferCompleteEx"), Browsable(false), EditorBrowsable(EditorBrowsableState.Never)]
        public event EventHandler TransferComplete;

        public event TransferHandler TransferCompleteEx;

        [EditorBrowsable(EditorBrowsableState.Never), Browsable(false), Obsolete("Use TransferStartedEx")]
        public event EventHandler TransferStarted;

        public event TransferHandler TransferStartedEx;

        static FTPClient()
        {
            try
            {
                version = new int[] { int.Parse(majorVersion), int.Parse(middleVersion), int.Parse(minorVersion) };
            }
            catch (FormatException exception)
            {
                Console.Error.WriteLine("Failed to calculate version: " + exception.Message);
            }
        }

        public FTPClient()
        {
            this.dirEmptyStrings = new DirectoryEmptyStrings();
            this.transferCompleteStrings = new TransferCompleteStrings();
            this.fileNotFoundStrings = new FileNotFoundStrings();
            this.modtimeFormats = new string[] { "yyyyMMddHHmmss", "yyyyMMddHHmmss'.'f", "yyyyMMddHHmmss'.'ff", "yyyyMMddHHmmss'.'fff" };
            this.control = null;
            this.data = null;
            this.timeout = 0x1d4c0;
            this.noOperationInterval = 0;
            this.strictReturnCodes = false;
            this.cancelTransfer = false;
            this.transferNotifyListings = false;
            this.resume = false;
            this.deleteOnFailure = true;
            this.mdtmSupported = true;
            this.sizeSupported = true;
            this.resumeMarker = 0L;
            this.showHiddenFiles = false;
            this.monitorInterval = 0x1000L;
            this.transferBufferSize = 0x1000;
            this.parserCulture = CultureInfo.InvariantCulture;
            this.fileFactory = new EnterpriseDT.Net.Ftp.FTPFileFactory();
            this.transferType = FTPTransferType.ASCII;
            this.connectMode = FTPConnectMode.PASV;
            this.synchronizePassiveConnections = false;
            this.activePortRange = new PortRange();
            this.activeIPAddress = null;
            this.controlPort = -1;
            this.remoteHost = null;
            this.autoPassiveIPSubstitution = false;
            this.closeStreamsAfterTransfer = true;
            this.controlEncoding = null;
            this.dataEncoding = null;
            this.throttler = null;
            this.InitBlock();
        }

        [Obsolete("This constructor is obsolete; use the default constructor and properties instead"), EditorBrowsable(EditorBrowsableState.Never)]
        public FTPClient(IPAddress remoteAddr) : this(remoteAddr, 0x15, 0)
        {
        }

        [Obsolete("This constructor is obsolete; use the default constructor and properties instead"), EditorBrowsable(EditorBrowsableState.Never)]
        public FTPClient(string remoteHost) : this(remoteHost, 0x15, 0)
        {
        }

        [Obsolete("This constructor is obsolete; use the default constructor and properties instead"), EditorBrowsable(EditorBrowsableState.Never)]
        public FTPClient(IPAddress remoteAddr, int controlPort) : this(remoteAddr, controlPort, 0)
        {
        }

        [EditorBrowsable(EditorBrowsableState.Never), Obsolete("This constructor is obsolete; use the default constructor and properties instead")]
        public FTPClient(string remoteHost, int controlPort) : this(remoteHost, controlPort, 0)
        {
        }

        [EditorBrowsable(EditorBrowsableState.Never), Obsolete("This constructor is obsolete; use the default constructor and properties instead")]
        public FTPClient(IPAddress remoteAddr, int controlPort, int timeout)
        {
            this.dirEmptyStrings = new DirectoryEmptyStrings();
            this.transferCompleteStrings = new TransferCompleteStrings();
            this.fileNotFoundStrings = new FileNotFoundStrings();
            this.modtimeFormats = new string[] { "yyyyMMddHHmmss", "yyyyMMddHHmmss'.'f", "yyyyMMddHHmmss'.'ff", "yyyyMMddHHmmss'.'fff" };
            this.control = null;
            this.data = null;
            this.timeout = 0x1d4c0;
            this.noOperationInterval = 0;
            this.strictReturnCodes = false;
            this.cancelTransfer = false;
            this.transferNotifyListings = false;
            this.resume = false;
            this.deleteOnFailure = true;
            this.mdtmSupported = true;
            this.sizeSupported = true;
            this.resumeMarker = 0L;
            this.showHiddenFiles = false;
            this.monitorInterval = 0x1000L;
            this.transferBufferSize = 0x1000;
            this.parserCulture = CultureInfo.InvariantCulture;
            this.fileFactory = new EnterpriseDT.Net.Ftp.FTPFileFactory();
            this.transferType = FTPTransferType.ASCII;
            this.connectMode = FTPConnectMode.PASV;
            this.synchronizePassiveConnections = false;
            this.activePortRange = new PortRange();
            this.activeIPAddress = null;
            this.controlPort = -1;
            this.remoteHost = null;
            this.autoPassiveIPSubstitution = false;
            this.closeStreamsAfterTransfer = true;
            this.controlEncoding = null;
            this.dataEncoding = null;
            this.throttler = null;
            this.InitBlock();
            this.remoteHost = remoteAddr.ToString();
            this.Connect(remoteAddr, controlPort, timeout);
        }

        [EditorBrowsable(EditorBrowsableState.Never), Obsolete("This constructor is obsolete; use the default constructor and properties instead")]
        public FTPClient(string remoteHost, int controlPort, int timeout) : this(HostNameResolver.GetAddress(remoteHost), controlPort, timeout)
        {
            this.remoteHost = remoteHost;
        }

        protected virtual void Abort()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("ABOR");
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "426", "226" });
        }

        public virtual void Account(string accountInfo)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("ACCT " + accountInfo);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "230", "202" });
        }

        public virtual void CancelResume()
        {
            this.Restart(0L);
            this.resume = false;
        }

        public virtual void CancelTransfer()
        {
            this.cancelTransfer = true;
        }

        public virtual void CdUp()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("CDUP");
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250" });
        }

        public virtual void ChDir(string dir)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("CWD " + dir);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250" });
        }

        internal void CheckConnection(bool shouldBeConnected)
        {
            if (shouldBeConnected && !this.Connected)
            {
                throw new FTPException("The FTP client has not yet connected to the server.  The requested action cannot be performed until after a connection has been established.");
            }
            if (!shouldBeConnected && this.Connected)
            {
                throw new FTPException("The FTP client has already been connected to the server.  The requested action must be performed before a connection is established.");
            }
        }

        private void CloseDataSocket()
        {
            if (this.data != null)
            {
                try
                {
                    this.data.Close();
                    this.data = null;
                }
                catch (SystemException exception)
                {
                    this.log.Warn("Caught exception closing data socket", exception);
                }
            }
        }

        protected void CloseDataSocket(Stream stream)
        {
            if (stream != null)
            {
                try
                {
                    stream.Close();
                }
                catch (IOException exception)
                {
                    this.log.Warn("Caught exception closing stream", exception);
                }
            }
            this.CloseDataSocket();
        }

        protected void CloseDataSocket(StreamReader stream)
        {
            if (stream != null)
            {
                try
                {
                    stream.Close();
                }
                catch (IOException exception)
                {
                    this.log.Warn("Caught exception closing stream", exception);
                }
            }
            this.CloseDataSocket();
        }

        protected void CloseDataSocket(StreamWriter stream)
        {
            if (stream != null)
            {
                try
                {
                    stream.Close();
                }
                catch (IOException exception)
                {
                    this.log.Warn("Caught exception closing stream", exception);
                }
            }
            this.CloseDataSocket();
        }

        internal void CommandSentControl(object client, FTPMessageEventArgs message)
        {
            if (this.CommandSent != null)
            {
                this.CommandSent(this, message);
            }
        }

        public virtual void Connect()
        {
            this.CheckConnection(false);
            if (this.remoteHost == null)
            {
                throw new FTPException("RemoteHost is not set.");
            }
            this.Connect(this.remoteHost, this.controlPort, this.timeout);
        }

        internal virtual void Connect(IPAddress remoteHost, int controlPort, int timeout)
        {
            this.Connect(remoteHost.ToString(), controlPort, timeout);
        }

        internal virtual void Connect(string remoteHost, int controlPort, int timeout)
        {
            if (controlPort < 0)
            {
                this.log.Warn(string.Concat(new object[] { "Invalid control port supplied: ", controlPort, " Using default: ", 0x15 }));
                controlPort = 0x15;
            }
            this.controlPort = controlPort;
            if (this.log.DebugEnabled)
            {
                this.log.Debug(string.Concat(new object[] { "Connecting to ", remoteHost, ":", controlPort }));
            }
            this.Initialize(new FTPControlSocket(remoteHost, controlPort, timeout, this.controlEncoding));
        }

        public void DebugResponses(bool on)
        {
            if (on)
            {
                Logger.CurrentLevel = Level.DEBUG;
            }
            else
            {
                Logger.CurrentLevel = Level.OFF;
            }
        }

        public virtual void Delete(string remoteFile)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("DELE " + remoteFile);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250" });
        }

        public virtual string[] Dir()
        {
            return this.Dir(null, false);
        }

        public virtual string[] Dir(string dirname)
        {
            return this.Dir(dirname, false);
        }

        public virtual string[] Dir(string dirname, bool full)
        {
            string[] strArray3;
            this.CheckConnection(true);
            try
            {
                this.data = this.control.CreateDataSocket(this.connectMode);
                this.data.Timeout = this.timeout;
                string command = full ? "LIST " : "NLST ";
                if (this.showHiddenFiles)
                {
                    command = command + "-a ";
                }
                if (dirname != null)
                {
                    command = command + dirname;
                }
                command = command.Trim();
                FTPReply reply = this.control.SendCommand(command);
                this.lastValidReply = this.control.ValidateReply(reply, new string[] { "125", "226", "150", "450", "550" });
                string[] array = new string[0];
                string replyCode = this.lastValidReply.ReplyCode;
                if ((!replyCode.Equals("450") && !replyCode.Equals("550")) && !replyCode.Equals("226"))
                {
                    Encoding enc = (this.controlEncoding == null) ? Encoding.ASCII : this.controlEncoding;
                    ArrayList list = null;
                    this.cancelTransfer = false;
                    try
                    {
                        if (enc.Equals(Encoding.ASCII))
                        {
                            list = this.ReadASCIIListingData(dirname);
                        }
                        else
                        {
                            list = this.ReadListingData(dirname, enc);
                        }
                        reply = this.control.ReadReply();
                        this.lastValidReply = this.control.ValidateReply(reply, new string[] { "226", "250" });
                    }
                    catch (SystemException exception)
                    {
                        this.ValidateTransferOnError();
                        this.log.Error("SystemException in directory listing", exception);
                        throw;
                    }
                    if (list.Count != 0)
                    {
                        this.log.Debug("Found " + list.Count + " listing lines");
                        array = new string[list.Count];
                        list.CopyTo(array);
                    }
                    else
                    {
                        this.log.Debug("No listing data found");
                    }
                }
                else
                {
                    string str3 = this.lastValidReply.ReplyText.ToUpper();
                    if (!this.dirEmptyStrings.Matches(str3) && !this.transferCompleteStrings.Matches(str3))
                    {
                        throw new FTPException(reply);
                    }
                }
                strArray3 = array;
            }
            finally
            {
                this.CloseDataSocket();
            }
            return strArray3;
        }

        public virtual FTPFile[] DirDetails()
        {
            return this.DirDetails(null);
        }

        public virtual FTPFile[] DirDetails(string dirname)
        {
            if (this.fileFactory == null)
            {
                this.fileFactory = new EnterpriseDT.Net.Ftp.FTPFileFactory();
            }
            if (!this.fileFactory.ParserSetExplicitly && (this.fileFactory.System == null))
            {
                try
                {
                    this.fileFactory.System = this.GetSystem();
                }
                catch (FTPException exception)
                {
                    this.log.Warn("SYST command failed - setting Unix as default parser", exception);
                    this.fileFactory.System = "UNIX";
                }
            }
            if (this.parserCulture != null)
            {
                this.fileFactory.ParsingCulture = this.parserCulture;
            }
            string str = this.Pwd();
            if (((dirname != null) && (dirname.Length > 0)) && ((dirname.IndexOf('*') < 0) && (dirname.IndexOf('?') < 0)))
            {
                str = Path.Combine(str, dirname);
            }
            FTPFile[] fileArray = this.fileFactory.Parse(this.Dir(dirname, true));
            for (int i = 0; i < fileArray.Length; i++)
            {
                fileArray[i].Path = str + (str.EndsWith("/") ? "" : "/") + fileArray[i].Name;
            }
            return fileArray;
        }

        public virtual bool Exists(string remoteFile)
        {
            char ch;
            this.CheckConnection(true);
            FTPReply reply = null;
            if (this.sizeSupported)
            {
                reply = this.control.SendCommand("SIZE " + remoteFile);
                ch = reply.ReplyCode[0];
                if (ch == '2')
                {
                    return true;
                }
                if ((ch == '5') && this.fileNotFoundStrings.Matches(reply.ReplyText))
                {
                    return false;
                }
                this.sizeSupported = false;
                this.log.Debug("SIZE not supported");
            }
            if (this.mdtmSupported)
            {
                reply = this.control.SendCommand("MDTM " + remoteFile);
                ch = reply.ReplyCode[0];
                if (ch == '2')
                {
                    return true;
                }
                if ((ch == '5') && this.fileNotFoundStrings.Matches(reply.ReplyText))
                {
                    return false;
                }
                this.mdtmSupported = false;
                this.log.Debug("MDTM not supported");
            }
            Socket socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            IPEndPoint localEP = new IPEndPoint(this.control.LocalAddress, 0);
            socket.Bind(localEP);
            try
            {
                this.control.SetDataPort((IPEndPoint) socket.LocalEndPoint);
            }
            finally
            {
                socket.Close();
            }
            reply = this.control.SendCommand("RETR " + remoteFile);
            ch = reply.ReplyCode[0];
            switch (ch)
            {
                case '1':
                case '2':
                case '4':
                    return true;
            }
            if ((ch == '5') && this.fileNotFoundStrings.Matches(reply.ReplyText))
            {
                return false;
            }
            string message = "Unable to determine if file '" + remoteFile + "' exists.";
            this.log.Warn(message);
            throw new FTPException(message);
        }

        public virtual string[] Features()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("FEAT");
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "211", "500", "502" });
            if (this.lastValidReply.ReplyCode != "211")
            {
                throw new FTPException(reply);
            }
            return this.lastValidReply.ReplyData;
        }

        public virtual byte[] Get(string remoteFile)
        {
            if (this.TransferStarted != null)
            {
                this.TransferStarted(this, new EventArgs());
            }
            if (this.TransferStartedEx != null)
            {
                this.TransferStartedEx(this, new TransferEventArgs(new byte[0], remoteFile, TransferDirection.DOWNLOAD, this.transferType));
            }
            this.InitGet(remoteFile);
            BufferedStream input = null;
            long bytesSoFar = 0L;
            Exception t = null;
            MemoryStream stream2 = null;
            byte[] localByteArray = null;
            try
            {
                int num3;
                input = new BufferedStream(this.GetInputStream());
                long num2 = 0L;
                byte[] chunk = new byte[this.transferBufferSize];
                stream2 = new MemoryStream(this.transferBufferSize);
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((num3 = this.ReadChunk(input, chunk, this.transferBufferSize)) > 0) && !this.cancelTransfer)
                {
                    stream2.Write(chunk, 0, num3);
                    bytesSoFar += num3;
                    num2 += num3;
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                try
                {
                    if (stream2 != null)
                    {
                        stream2.Close();
                    }
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing stream", exception3);
                }
                this.CloseDataSocket(input);
                if (t != null)
                {
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                }
                this.ValidateTransfer();
                localByteArray = (stream2 == null) ? null : stream2.ToArray();
                if (this.TransferComplete != null)
                {
                    this.TransferComplete(this, new EventArgs());
                }
                if (this.TransferCompleteEx != null)
                {
                    this.TransferCompleteEx(this, new TransferEventArgs(localByteArray, remoteFile, TransferDirection.UPLOAD, this.transferType));
                }
            }
            return localByteArray;
        }

        public virtual void Get(Stream destStream, string remoteFile)
        {
            if (this.TransferStarted != null)
            {
                this.TransferStarted(this, new EventArgs());
            }
            if (this.TransferStartedEx != null)
            {
                this.TransferStartedEx(this, new TransferEventArgs(destStream, remoteFile, TransferDirection.DOWNLOAD, this.transferType));
            }
            try
            {
                if (this.transferType == FTPTransferType.ASCII)
                {
                    this.GetASCII(destStream, remoteFile);
                }
                else
                {
                    this.GetBinary(destStream, remoteFile);
                }
            }
            catch (SystemException exception)
            {
                this.ValidateTransferOnError();
                this.log.Error("SystemException in Get(Stream,string)", exception);
                throw;
            }
            this.ValidateTransfer();
            if (this.TransferComplete != null)
            {
                this.TransferComplete(this, new EventArgs());
            }
            if (this.TransferCompleteEx != null)
            {
                this.TransferCompleteEx(this, new TransferEventArgs(destStream, remoteFile, TransferDirection.DOWNLOAD, this.transferType));
            }
        }

        public virtual void Get(string localPath, string remoteFile)
        {
            if (Directory.Exists(localPath))
            {
                localPath = Path.Combine(localPath, remoteFile);
                this.log.Debug("Setting local path to " + localPath);
            }
            if (this.TransferStarted != null)
            {
                this.TransferStarted(this, new EventArgs());
            }
            if (this.TransferStartedEx != null)
            {
                TransferEventArgs e = new TransferEventArgs(localPath, remoteFile, TransferDirection.DOWNLOAD, this.transferType);
                this.TransferStartedEx(this, e);
                localPath = e.LocalFilePath;
            }
            try
            {
                if (this.transferType == FTPTransferType.ASCII)
                {
                    this.GetASCII(localPath, remoteFile);
                }
                else
                {
                    this.GetBinary(localPath, remoteFile);
                }
            }
            catch (SystemException exception)
            {
                this.ValidateTransferOnError();
                this.log.Error("SystemException in Get(string,string)", exception);
                throw;
            }
            this.ValidateTransfer();
            if (this.TransferComplete != null)
            {
                this.TransferComplete(this, new EventArgs());
            }
            if (this.TransferCompleteEx != null)
            {
                this.TransferCompleteEx(this, new TransferEventArgs(localPath, remoteFile, TransferDirection.DOWNLOAD, this.transferType));
            }
        }

        private void GetASCII(Stream destStream, string remoteFile)
        {
            this.InitGet(remoteFile);
            StreamWriter writer = (this.dataEncoding == null) ? new StreamWriter(destStream) : new StreamWriter(destStream, this.dataEncoding);
            StreamReader input = null;
            Exception t = null;
            long bytesSoFar = 0L;
            try
            {
                input = (this.dataEncoding == null) ? new StreamReader(this.GetInputStream()) : new StreamReader(this.GetInputStream(), this.dataEncoding);
                long num2 = 0L;
                string str = null;
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((str = this.ReadLine(input)) != null) && !this.cancelTransfer)
                {
                    bytesSoFar += str.Length + 2;
                    num2 += str.Length + 2;
                    writer.WriteLine(str);
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, 0L));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
                writer.Flush();
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                try
                {
                    if (this.closeStreamsAfterTransfer)
                    {
                        writer.Close();
                    }
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing output stream", exception3);
                }
                this.CloseDataSocket(input);
                if (t != null)
                {
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, 0L));
                }
            }
        }

        private void GetASCII(string localPath, string remoteFile)
        {
            FileInfo info = new FileInfo(localPath);
            StreamWriter writer = null;
            if (info.Exists)
            {
                if ((info.Attributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
                {
                    throw new FTPException(localPath + " is readonly - cannot write");
                }
                writer = (this.dataEncoding == null) ? new StreamWriter(localPath) : new StreamWriter(localPath, false, this.dataEncoding);
            }
            if (writer == null)
            {
                writer = (this.dataEncoding == null) ? new StreamWriter(localPath) : new StreamWriter(localPath, false, this.dataEncoding);
            }
            Exception t = null;
            long bytesSoFar = 0L;
            StreamReader input = null;
            try
            {
                this.InitGet(remoteFile);
                input = (this.dataEncoding == null) ? new StreamReader(this.GetInputStream()) : new StreamReader(this.GetInputStream(), this.dataEncoding);
                long num2 = 0L;
                string str = null;
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((str = this.ReadLine(input)) != null) && !this.cancelTransfer)
                {
                    bytesSoFar += str.Length + 2;
                    num2 += str.Length + 2;
                    writer.WriteLine(str);
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, 0L));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                try
                {
                    writer.Close();
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing output stream", exception3);
                }
                this.CloseDataSocket(input);
                if (t != null)
                {
                    if (this.deleteOnFailure)
                    {
                        info.Delete();
                    }
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, 0L));
                }
            }
        }

        private void GetBinary(Stream destStream, string remoteFile)
        {
            this.InitGet(remoteFile);
            BufferedStream stream = new BufferedStream(destStream);
            BufferedStream input = null;
            long bytesSoFar = 0L;
            Exception t = null;
            try
            {
                int num3;
                input = new BufferedStream(this.GetInputStream());
                long num2 = 0L;
                byte[] chunk = new byte[this.transferBufferSize];
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((num3 = this.ReadChunk(input, chunk, this.transferBufferSize)) > 0) && !this.cancelTransfer)
                {
                    stream.Write(chunk, 0, num3);
                    bytesSoFar += num3;
                    num2 += num3;
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
                stream.Flush();
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                try
                {
                    if (this.closeStreamsAfterTransfer)
                    {
                        stream.Close();
                    }
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing output stream", exception3);
                }
                this.CloseDataSocket(input);
                if (t != null)
                {
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                }
                this.log.Debug("Transferred " + bytesSoFar + " bytes from remote host");
            }
        }

        private void GetBinary(string localPath, string remoteFile)
        {
            FileInfo info = new FileInfo(localPath);
            FileMode mode = this.resume ? FileMode.Append : FileMode.Create;
            BufferedStream stream = null;
            if (info.Exists)
            {
                if ((info.Attributes & FileAttributes.ReadOnly) == FileAttributes.ReadOnly)
                {
                    throw new FTPException(localPath + " is readonly - cannot write");
                }
                if (this.resume)
                {
                    this.resumeMarker = info.Length;
                }
                else
                {
                    this.resumeMarker = 0L;
                }
                stream = new BufferedStream(new FileStream(localPath, mode));
            }
            if (stream == null)
            {
                stream = new BufferedStream(new FileStream(localPath, mode));
            }
            BufferedStream input = null;
            long bytesSoFar = 0L;
            Exception t = null;
            try
            {
                int num3;
                this.InitGet(remoteFile);
                input = new BufferedStream(this.GetInputStream());
                long num2 = 0L;
                byte[] chunk = new byte[this.transferBufferSize];
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((num3 = this.ReadChunk(input, chunk, this.transferBufferSize)) > 0) && !this.cancelTransfer)
                {
                    stream.Write(chunk, 0, num3);
                    bytesSoFar += num3;
                    num2 += num3;
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                this.resume = false;
                try
                {
                    stream.Close();
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing output stream", exception3);
                }
                this.CloseDataSocket(input);
                if (t != null)
                {
                    if (this.deleteOnFailure)
                    {
                        info.Delete();
                    }
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                }
                this.log.Debug("Transferred " + bytesSoFar + " bytes from remote host");
            }
        }

        protected virtual Stream GetInputStream()
        {
            return this.data.DataStream;
        }

        protected virtual Stream GetOutputStream()
        {
            return this.data.DataStream;
        }

        internal string GetPASVAddress(string pasvReply)
        {
            int num2;
            int startIndex = -1;
            for (num2 = 0; num2 < pasvReply.Length; num2++)
            {
                if (char.IsDigit(pasvReply[num2]))
                {
                    startIndex = num2;
                    break;
                }
            }
            int num3 = -1;
            for (num2 = pasvReply.Length - 1; num2 >= 0; num2--)
            {
                if (char.IsDigit(pasvReply[num2]))
                {
                    num3 = num2;
                    break;
                }
            }
            if ((startIndex < 0) || (num3 < 0))
            {
                return null;
            }
            int length = (num3 - startIndex) + 1;
            return pasvReply.Substring(startIndex, length);
        }

        public virtual string GetSystem()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("SYST");
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "213", "215", "250" });
            return this.lastValidReply.ReplyText;
        }

        public virtual string Help(string command)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("HELP " + command);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "211", "214" });
            return this.lastValidReply.ReplyText;
        }

        private void InitBlock()
        {
            this.log = Logger.GetLogger("FTPClient");
            this.transferType = FTPTransferType.ASCII;
            this.connectMode = FTPConnectMode.PASV;
            this.controlPort = 0x15;
        }

        private void InitGet(string remoteFile)
        {
            this.CheckConnection(true);
            this.cancelTransfer = false;
            bool flag = false;
            this.data = null;
            try
            {
                this.data = this.control.CreateDataSocket(this.connectMode);
                this.data.Timeout = this.timeout;
                if (this.resume)
                {
                    if (this.transferType.Equals(FTPTransferType.ASCII))
                    {
                        throw new FTPException("Resume only supported for BINARY transfers");
                    }
                    this.Restart(this.resumeMarker);
                }
                else
                {
                    this.resumeMarker = 0L;
                }
                FTPReply reply = this.control.SendCommand("RETR " + remoteFile);
                this.lastValidReply = this.control.ValidateReply(reply, new string[] { "125", "150" });
            }
            catch (SystemException)
            {
                flag = true;
                throw;
            }
            catch (FTPException)
            {
                flag = true;
                throw;
            }
            finally
            {
                if (flag)
                {
                    this.resume = false;
                    this.CloseDataSocket();
                }
            }
        }

        internal void Initialize(FTPControlSocket control)
        {
            this.control = control;
            this.control.AutoPassiveIPSubstitution = this.autoPassiveIPSubstitution;
            this.control.SynchronizePassiveConnections = this.synchronizePassiveConnections;
            control.CommandSent += new FTPMessageHandler(this.CommandSentControl);
            control.ReplyReceived += new FTPMessageHandler(this.ReplyReceivedControl);
            if (this.activePortRange != null)
            {
                control.SetActivePortRange(this.activePortRange);
            }
            if (this.activeIPAddress != null)
            {
                control.SetActiveIPAddress(this.activeIPAddress);
            }
            control.StrictReturnCodes = this.strictReturnCodes;
            this.control.ValidateConnection();
        }

        private void InitPut(string remoteFile, bool append)
        {
            this.CheckConnection(true);
            this.cancelTransfer = false;
            bool flag = false;
            this.data = null;
            try
            {
                this.resumeMarker = 0L;
                if (this.resume)
                {
                    if (this.transferType.Equals(FTPTransferType.ASCII))
                    {
                        throw new FTPException("Resume only supported for BINARY transfers");
                    }
                    try
                    {
                        this.resumeMarker = this.Size(remoteFile);
                    }
                    catch (FTPException exception)
                    {
                        this.log.Warn("Failed to find size of file '" + remoteFile + "' for resuming (" + exception.Message + ")");
                    }
                }
                this.data = this.control.CreateDataSocket(this.connectMode);
                this.data.Timeout = this.timeout;
                if (this.resume)
                {
                    this.Restart(this.resumeMarker);
                }
                FTPReply reply = this.control.SendCommand((append ? "APPE " : "STOR ") + remoteFile);
                this.lastValidReply = this.control.ValidateReply(reply, new string[] { "125", "150", "151", "350" });
            }
            catch (SystemException)
            {
                flag = true;
                throw;
            }
            catch (FTPException)
            {
                flag = true;
                throw;
            }
            finally
            {
                if (flag)
                {
                    this.resume = false;
                    this.CloseDataSocket();
                }
            }
        }

        public virtual void Login(string user, string password)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("USER " + user);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "230", "331" });
            if (!this.lastValidReply.ReplyCode.Equals("230"))
            {
                this.Password(password);
            }
        }

        public virtual void MkDir(string dir)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("MKD " + dir);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250", "257" });
        }

        public virtual DateTime ModTime(string remoteFile)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("MDTM " + remoteFile);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "213" });
            DateTime time = DateTime.ParseExact(this.lastValidReply.ReplyText, this.modtimeFormats, null, DateTimeStyles.None);
            return TimeZone.CurrentTimeZone.ToLocalTime(time);
        }

        public void NoOperation()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("NOOP");
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250" });
        }

        public virtual void Password(string password)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("PASS " + password);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "230", "202", "332" });
        }

        public virtual void Put(byte[] bytes, string remoteFile)
        {
            this.Put(bytes, remoteFile, false);
        }

        public virtual long Put(Stream srcStream, string remoteFile)
        {
            return this.Put(srcStream, remoteFile, false);
        }

        public virtual void Put(string localPath, string remoteFile)
        {
            this.Put(localPath, remoteFile, false);
        }

        public virtual long Put(Stream srcStream, string remoteFile, bool append)
        {
            if (this.TransferStarted != null)
            {
                this.TransferStarted(this, new EventArgs());
            }
            if (this.TransferStartedEx != null)
            {
                this.TransferStartedEx(this, new TransferEventArgs(srcStream, remoteFile, TransferDirection.UPLOAD, this.transferType));
            }
            long num = 0L;
            try
            {
                if (this.transferType == FTPTransferType.ASCII)
                {
                    num = this.PutASCII(srcStream, remoteFile, append, false);
                }
                else
                {
                    num = this.PutBinary(srcStream, remoteFile, append, false);
                }
            }
            catch (SystemException exception)
            {
                this.log.Error("SystemException in Put(Stream,string,bool)", exception);
                this.ValidateTransferOnError();
                throw;
            }
            this.ValidateTransfer();
            if (this.TransferComplete != null)
            {
                this.TransferComplete(this, new EventArgs());
            }
            if (this.TransferCompleteEx != null)
            {
                this.TransferCompleteEx(this, new TransferEventArgs(srcStream, remoteFile, TransferDirection.UPLOAD, this.transferType));
            }
            return num;
        }

        public virtual void Put(byte[] bytes, string remoteFile, bool append)
        {
            MemoryStream srcStream = new MemoryStream(bytes);
            this.Put(srcStream, remoteFile, append);
            srcStream.Close();
        }

        public virtual void Put(string localPath, string remoteFile, bool append)
        {
            if (this.TransferStarted != null)
            {
                this.TransferStarted(this, new EventArgs());
            }
            if (this.TransferStartedEx != null)
            {
                this.TransferStartedEx(this, new TransferEventArgs(localPath, remoteFile, TransferDirection.UPLOAD, this.transferType));
            }
            try
            {
                if (this.transferType == FTPTransferType.ASCII)
                {
                    this.PutASCII(localPath, remoteFile, append);
                }
                else
                {
                    this.PutBinary(localPath, remoteFile, append);
                }
            }
            catch (SystemException exception)
            {
                this.log.Error("SystemException in Put(string,string,bool)", exception);
                this.ValidateTransferOnError();
                throw;
            }
            this.ValidateTransfer();
            if (this.TransferComplete != null)
            {
                this.TransferComplete(this, new EventArgs());
            }
            if (this.TransferCompleteEx != null)
            {
                this.TransferCompleteEx(this, new TransferEventArgs(localPath, remoteFile, TransferDirection.UPLOAD, this.transferType));
            }
        }

        private void PutASCII(string localPath, string remoteFile, bool append)
        {
            Stream srcStream = null;
            try
            {
                srcStream = new FileStream(localPath, FileMode.Open, FileAccess.Read);
            }
            catch (Exception exception)
            {
                string message = "Failed to open file '" + localPath + "'";
                this.log.Error(message, exception);
                throw new FTPException(message);
            }
            this.PutASCII(srcStream, remoteFile, append, true);
        }

        private long PutASCII(Stream srcStream, string remoteFile, bool append, bool alwaysCloseStreams)
        {
            StreamReader reader = null;
            StreamWriter stream = null;
            Exception t = null;
            long bytesSoFar = 0L;
            try
            {
                reader = (this.dataEncoding == null) ? new StreamReader(srcStream) : new StreamReader(srcStream, this.dataEncoding);
                this.InitPut(remoteFile, append);
                stream = (this.dataEncoding == null) ? new StreamWriter(this.GetOutputStream()) : new StreamWriter(this.GetOutputStream(), this.dataEncoding);
                long num2 = 0L;
                string str = null;
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((str = reader.ReadLine()) != null) && !this.cancelTransfer)
                {
                    bytesSoFar += str.Length + 2;
                    num2 += str.Length + 2;
                    stream.Write(str);
                    stream.Write("\r\n");
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, 0L));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                try
                {
                    if (alwaysCloseStreams || this.closeStreamsAfterTransfer)
                    {
                        this.log.Debug("Closing source stream");
                        srcStream.Close();
                        if (reader != null)
                        {
                            reader.Close();
                        }
                    }
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing stream", exception3);
                }
                try
                {
                    if (stream != null)
                    {
                        stream.Flush();
                    }
                }
                catch (SystemException exception4)
                {
                    this.log.Warn("Caught exception flushing output-stream", exception4);
                }
                this.CloseDataSocket(stream);
                if (t != null)
                {
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, 0L));
                }
            }
            return bytesSoFar;
        }

        private void PutBinary(string localPath, string remoteFile, bool append)
        {
            Stream srcStream = null;
            try
            {
                srcStream = new FileStream(localPath, FileMode.Open, FileAccess.Read);
            }
            catch (Exception exception)
            {
                string message = "Failed to open file '" + localPath + "'";
                this.log.Error(message, exception);
                throw new FTPException(message);
            }
            this.PutBinary(srcStream, remoteFile, append, true);
        }

        private long PutBinary(Stream srcStream, string remoteFile, bool append, bool alwaysCloseStreams)
        {
            BufferedStream stream = null;
            BufferedStream stream2 = null;
            Exception t = null;
            long bytesSoFar = 0L;
            try
            {
                stream = new BufferedStream(srcStream);
                this.InitPut(remoteFile, append);
                stream2 = new BufferedStream(this.GetOutputStream());
                if (this.resume)
                {
                    stream.Seek(this.resumeMarker, SeekOrigin.Current);
                }
                else
                {
                    this.resumeMarker = 0L;
                }
                byte[] buffer = new byte[this.transferBufferSize];
                long num2 = 0L;
                int count = 0;
                DateTime now = DateTime.Now;
                if (this.throttler != null)
                {
                    this.throttler.Reset();
                }
                while (((count = stream.Read(buffer, 0, buffer.Length)) > 0) && !this.cancelTransfer)
                {
                    stream2.Write(buffer, 0, count);
                    bytesSoFar += count;
                    num2 += count;
                    if (this.throttler != null)
                    {
                        this.throttler.ThrottleTransfer(bytesSoFar);
                    }
                    if (((this.BytesTransferred != null) && !this.cancelTransfer) && (num2 >= this.monitorInterval))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                        num2 = 0L;
                    }
                    now = this.SendServerWakeup(now);
                }
            }
            catch (Exception exception2)
            {
                t = exception2;
            }
            finally
            {
                this.resume = false;
                try
                {
                    if (alwaysCloseStreams || this.closeStreamsAfterTransfer)
                    {
                        this.log.Debug("Closing source stream");
                        if (stream != null)
                        {
                            stream.Close();
                        }
                    }
                }
                catch (SystemException exception3)
                {
                    this.log.Warn("Caught exception closing stream", exception3);
                }
                try
                {
                    if (stream2 != null)
                    {
                        stream2.Flush();
                    }
                }
                catch (SystemException exception4)
                {
                    this.log.Warn("Caught exception flushing output-stream", exception4);
                }
                this.CloseDataSocket(stream2);
                if (t != null)
                {
                    this.log.Error("Caught exception", t);
                    throw t;
                }
                if ((this.BytesTransferred != null) && !this.cancelTransfer)
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(remoteFile, bytesSoFar, this.resumeMarker));
                }
                this.log.Debug("Transferred " + bytesSoFar + " bytes to remote host");
            }
            return bytesSoFar;
        }

        public virtual string Pwd()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("PWD");
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "257" });
            string replyText = this.lastValidReply.ReplyText;
            int index = replyText.IndexOf('"');
            int num2 = replyText.LastIndexOf('"');
            if ((index >= 0) && (num2 > index))
            {
                return replyText.Substring(index + 1, num2 - (index + 1));
            }
            return replyText;
        }

        public virtual void Quit()
        {
            this.CheckConnection(true);
            if (this.fileFactory != null)
            {
                this.fileFactory.System = null;
            }
            try
            {
                FTPReply reply = this.control.SendCommand("QUIT");
                this.lastValidReply = this.control.ValidateReply(reply, new string[] { "221", "226" });
            }
            finally
            {
                if (this.data != null)
                {
                    this.data.Close();
                }
                this.data = null;
                this.control.Logout();
                this.control = null;
            }
        }

        public virtual void QuitImmediately()
        {
            if (this.fileFactory != null)
            {
                this.fileFactory.System = null;
            }
            try
            {
                if (this.data != null)
                {
                    this.data.Close();
                }
            }
            finally
            {
                if (this.control != null)
                {
                    this.control.Kill();
                }
                this.control = null;
                this.data = null;
            }
        }

        public virtual string Quote(string command, string[] validCodes)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand(command);
            if (validCodes != null)
            {
                this.lastValidReply = this.control.ValidateReply(reply, validCodes);
            }
            else
            {
                this.lastValidReply = reply;
            }
            return this.lastValidReply.ReplyText;
        }

        private ArrayList ReadASCIIListingData(string dirname)
        {
            this.log.Debug("Reading ASCII listing data");
            BufferedStream stream = new BufferedStream(this.GetInputStream());
            MemoryStream stream2 = new MemoryStream(this.TransferBufferSize * 2);
            long byteCount = 0L;
            long num2 = 0L;
            try
            {
                int num3;
                while (((num3 = stream.ReadByte()) != -1) && !this.cancelTransfer)
                {
                    if ((((num3 >= 0x20) || (num3 == 10)) || (num3 == 13)) && (num3 <= 0x7f))
                    {
                        byteCount += 1L;
                        num2 += 1L;
                        stream2.WriteByte((byte) num3);
                        if ((this.transferNotifyListings && (this.BytesTransferred != null)) && (!this.cancelTransfer && (num2 >= this.monitorInterval)))
                        {
                            this.BytesTransferred(this, new BytesTransferredEventArgs(dirname, byteCount, 0L));
                            num2 = 0L;
                        }
                    }
                }
                if (this.transferNotifyListings && (this.BytesTransferred != null))
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(dirname, byteCount, 0L));
                }
            }
            finally
            {
                this.CloseDataSocket(stream);
            }
            stream2.Seek(0L, SeekOrigin.Begin);
            StreamReader input = new StreamReader(stream2, Encoding.ASCII);
            ArrayList list = new ArrayList(10);
            string str = null;
            while ((str = this.ReadLine(input)) != null)
            {
                list.Add(str);
                this.log.Debug("-->" + str);
            }
            input.Close();
            stream2.Close();
            return list;
        }

        internal virtual int ReadChar(StreamReader input)
        {
            return input.Read();
        }

        internal virtual int ReadChunk(Stream input, byte[] chunk, int chunksize)
        {
            return input.Read(chunk, 0, chunksize);
        }

        internal virtual string ReadLine(StreamReader input)
        {
            return input.ReadLine();
        }

        private ArrayList ReadListingData(string dirname, Encoding enc)
        {
            ArrayList list2;
            StreamReader input = new StreamReader(this.GetInputStream(), enc);
            ArrayList list = new ArrayList(10);
            string str = null;
            long byteCount = 0L;
            long num2 = 0L;
            try
            {
                while (((str = this.ReadLine(input)) != null) && !this.cancelTransfer)
                {
                    byteCount += str.Length;
                    num2 += str.Length;
                    list.Add(str);
                    if ((this.transferNotifyListings && (this.BytesTransferred != null)) && (!this.cancelTransfer && (num2 >= this.monitorInterval)))
                    {
                        this.BytesTransferred(this, new BytesTransferredEventArgs(dirname, byteCount, 0L));
                        num2 = 0L;
                    }
                    this.log.Debug("-->" + str);
                }
                if (this.transferNotifyListings && (this.BytesTransferred != null))
                {
                    this.BytesTransferred(this, new BytesTransferredEventArgs(dirname, byteCount, 0L));
                }
                list2 = list;
            }
            finally
            {
                this.CloseDataSocket(input);
            }
            return list2;
        }

        public virtual void Rename(string from, string to)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("RNFR " + from);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "350" });
            reply = this.control.SendCommand("RNTO " + to);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "250" });
        }

        internal void ReplyReceivedControl(object client, FTPMessageEventArgs message)
        {
            if (this.ReplyReceived != null)
            {
                this.ReplyReceived(this, message);
            }
        }

        public void Restart(long size)
        {
            FTPReply reply = this.control.SendCommand("REST " + size);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "350" });
        }

        public virtual void Resume()
        {
            if (this.transferType.Equals(FTPTransferType.ASCII))
            {
                throw new FTPException("Resume only supported for BINARY transfers");
            }
            this.resume = true;
        }

        public virtual void RmDir(string dir)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("RMD " + dir);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250", "257" });
        }

        internal FTPReply SendCommand(string command)
        {
            return this.control.SendCommand(command);
        }

        private DateTime SendServerWakeup(DateTime start)
        {
            if (this.noOperationInterval != 0)
            {
                TimeSpan span = (TimeSpan) (DateTime.Now - start);
                int totalSeconds = (int) span.TotalSeconds;
                if (totalSeconds >= this.noOperationInterval)
                {
                    this.log.Info("Sending server wakeup message");
                    this.control.WriteCommand("NOOP");
                    return DateTime.Now;
                }
            }
            return start;
        }

        public virtual void SetModTime(string remoteFile, DateTime modTime)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("MFMT " + TimeZone.CurrentTimeZone.ToUniversalTime(modTime).ToString("yyyyMMddHHmmss") + " " + remoteFile);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "213" });
        }

        public virtual bool Site(string command)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("SITE " + command);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "202", "250", "502" });
            return reply.ReplyCode.Equals("200");
        }

        public virtual long Size(string remoteFile)
        {
            long num2;
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("SIZE " + remoteFile);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "213" });
            string replyText = this.lastValidReply.ReplyText;
            int index = replyText.IndexOf(' ');
            if (index >= 0)
            {
                replyText = replyText.Substring(0, index);
            }
            try
            {
                num2 = long.Parse(replyText);
            }
            catch (FormatException)
            {
                throw new FTPException("Failed to parse reply: " + replyText);
            }
            return num2;
        }

        public virtual void User(string user)
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.SendCommand("USER " + user);
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "230", "331" });
        }

        internal void ValidateReply(FTPReply reply, string[] expectedReplyCodes)
        {
            this.control.ValidateReply(reply, expectedReplyCodes);
        }

        internal void ValidateReply(FTPReply reply, string expectedReplyCode)
        {
            this.control.ValidateReply(reply, new string[] { expectedReplyCode });
        }

        public void ValidateTransfer()
        {
            this.CheckConnection(true);
            FTPReply reply = this.control.ReadReply();
            if (this.cancelTransfer)
            {
                this.lastValidReply = reply;
                this.log.Warn("Transfer cancelled");
                throw new FTPTransferCancelledException("Transfer cancelled.");
            }
            this.lastValidReply = this.control.ValidateReply(reply, new string[] { "225", "226", "250" });
        }

        protected void ValidateTransferOnError()
        {
            try
            {
                this.CheckConnection(true);
                if (this.control != null)
                {
                    this.control.Timeout = SHORT_TIMEOUT;
                }
                this.ValidateTransfer();
            }
            catch (Exception exception)
            {
                this.log.Error("Exception in ValidateTransferOnError())", exception);
            }
            finally
            {
                if (this.control != null)
                {
                    this.control.Timeout = this.timeout;
                }
            }
        }

        public IPAddress ActiveIPAddress
        {
            get
            {
                return this.activeIPAddress;
            }
            set
            {
                this.activeIPAddress = value;
                if (this.control != null)
                {
                    this.control.SetActiveIPAddress(value);
                }
            }
        }

        public PortRange ActivePortRange
        {
            get
            {
                return this.activePortRange;
            }
            set
            {
                value.ValidateRange();
                this.activePortRange = value;
                if (this.control != null)
                {
                    this.control.SetActivePortRange(value);
                }
            }
        }

        public bool AutoPassiveIPSubstitution
        {
            get
            {
                return this.autoPassiveIPSubstitution;
            }
            set
            {
                this.autoPassiveIPSubstitution = value;
                if (this.control != null)
                {
                    this.control.AutoPassiveIPSubstitution = value;
                }
            }
        }

        public static string BuildTimestamp
        {
            get
            {
                return buildTimestamp;
            }
        }

        public bool CloseStreamsAfterTransfer
        {
            get
            {
                return this.closeStreamsAfterTransfer;
            }
            set
            {
                this.closeStreamsAfterTransfer = value;
            }
        }

        public bool Connected
        {
            get
            {
                return ((this.control != null) && this.control.Connected);
            }
        }

        public FTPConnectMode ConnectMode
        {
            get
            {
                return this.connectMode;
            }
            set
            {
                this.connectMode = value;
            }
        }

        public Encoding ControlEncoding
        {
            get
            {
                return this.controlEncoding;
            }
            set
            {
                this.controlEncoding = value;
            }
        }

        public int ControlPort
        {
            get
            {
                return this.controlPort;
            }
            set
            {
                this.CheckConnection(false);
                this.controlPort = value;
            }
        }

        public Encoding DataEncoding
        {
            get
            {
                return this.dataEncoding;
            }
            set
            {
                this.dataEncoding = value;
            }
        }

        public bool DeleteOnFailure
        {
            get
            {
                return this.deleteOnFailure;
            }
            set
            {
                this.deleteOnFailure = value;
            }
        }

        public DirectoryEmptyStrings DirectoryEmptyMessages
        {
            get
            {
                return this.dirEmptyStrings;
            }
        }

        public FileNotFoundStrings FileNotFoundMessages
        {
            get
            {
                return this.fileNotFoundStrings;
            }
        }

        public EnterpriseDT.Net.Ftp.FTPFileFactory FTPFileFactory
        {
            get
            {
                return this.fileFactory;
            }
            set
            {
                this.fileFactory = value;
            }
        }

        public bool IsConnected
        {
            get
            {
                return this.Connected;
            }
        }

        public FTPReply LastValidReply
        {
            get
            {
                return this.lastValidReply;
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
                if (value == null)
                {
                    value = CultureInfo.InvariantCulture;
                }
                this.parserCulture = value;
                if (this.fileFactory != null)
                {
                    this.fileFactory.ParsingCulture = value;
                }
            }
        }

        public virtual string RemoteHost
        {
            get
            {
                return this.remoteHost;
            }
            set
            {
                this.CheckConnection(false);
                this.remoteHost = value;
            }
        }

        public int ServerWakeupInterval
        {
            get
            {
                return this.noOperationInterval;
            }
            set
            {
                this.noOperationInterval = value;
            }
        }

        public bool ShowHiddenFiles
        {
            get
            {
                return this.showHiddenFiles;
            }
            set
            {
                this.showHiddenFiles = value;
            }
        }

        public bool StrictReturnCodes
        {
            get
            {
                return this.strictReturnCodes;
            }
            set
            {
                this.strictReturnCodes = value;
                if (this.control != null)
                {
                    this.control.StrictReturnCodes = value;
                }
            }
        }

        public bool SynchronizePassiveConnections
        {
            get
            {
                return this.synchronizePassiveConnections;
            }
            set
            {
                this.synchronizePassiveConnections = value;
                if (this.control != null)
                {
                    this.control.SynchronizePassiveConnections = value;
                }
            }
        }

        public TimeSpan TimeDifference
        {
            get
            {
                if (this.fileFactory == null)
                {
                    return new TimeSpan();
                }
                return this.fileFactory.TimeDifference;
            }
            set
            {
                this.fileFactory.TimeDifference = value;
            }
        }

        public bool TimeIncludesSeconds
        {
            get
            {
                return this.fileFactory.TimeIncludesSeconds;
            }
        }

        public virtual int Timeout
        {
            get
            {
                return this.timeout;
            }
            set
            {
                this.timeout = value;
                if (this.control != null)
                {
                    this.control.Timeout = value;
                }
            }
        }

        public int TransferBufferSize
        {
            get
            {
                return this.transferBufferSize;
            }
            set
            {
                if (value <= 0)
                {
                    throw new ArgumentException("TransferBufferSize must be greater than 0.");
                }
                this.transferBufferSize = value;
            }
        }

        public TransferCompleteStrings TransferCompleteMessages
        {
            get
            {
                return this.transferCompleteStrings;
            }
        }

        public long TransferNotifyInterval
        {
            get
            {
                return this.monitorInterval;
            }
            set
            {
                this.monitorInterval = value;
            }
        }

        public bool TransferNotifyListings
        {
            get
            {
                return this.transferNotifyListings;
            }
            set
            {
                this.transferNotifyListings = value;
            }
        }

        public FTPTransferType TransferType
        {
            get
            {
                return this.transferType;
            }
            set
            {
                this.CheckConnection(true);
                string str = ASCII_CHAR;
                if (value.Equals(FTPTransferType.BINARY))
                {
                    str = BINARY_CHAR;
                }
                FTPReply reply = this.control.SendCommand("TYPE " + str);
                this.lastValidReply = this.control.ValidateReply(reply, new string[] { "200", "250" });
                this.transferType = value;
            }
        }

        public static int[] Version
        {
            get
            {
                return version;
            }
        }
    }
}

