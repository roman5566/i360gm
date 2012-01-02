namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Net;
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Collections;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Runtime.CompilerServices;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;

    public class FTPControlSocket
    {
        private IPAddress activeIPAddress;
        private PortRange activePortRange;
        private bool autoPassiveIPSubstitution;
        private const byte CARRIAGE_RETURN = 13;
        public const int CONTROL_PORT = 0x15;
        protected int controlPort;
        protected BaseSocket controlSock;
        private const string DEBUG_ARROW = "---> ";
        private Encoding encoding;
        internal const string EOL = "\r\n";
        private const byte LINE_FEED = 10;
        private Logger log;
        internal const int MAX_ACTIVE_RETRY = 100;
        private int nextPort;
        private static readonly string PASSWORD_MESSAGE = "---> PASS";
        private static string PASV_MUTEX_NAME = @"Global\edtFTPnet_SynchronizePassiveConnections";
        protected StreamReader reader;
        protected IPAddress remoteAddr;
        protected string remoteHost;
        private bool strictReturnCodes;
        private bool synchronizePassiveConnections;
        protected int timeout;
        protected StreamWriter writer;

        internal event FTPErrorEventHandler CommandError;

        internal event FTPMessageHandler CommandSent;

        internal event FTPMessageHandler ReplyReceived;

        internal FTPControlSocket()
        {
            this.log = Logger.GetLogger("FTPControlSocket");
            this.synchronizePassiveConnections = false;
            this.strictReturnCodes = false;
            this.remoteHost = null;
            this.remoteAddr = null;
            this.controlPort = -1;
            this.controlSock = null;
            this.timeout = 0;
            this.writer = null;
            this.reader = null;
            this.activePortRange = null;
            this.activeIPAddress = null;
            this.nextPort = 0;
            this.autoPassiveIPSubstitution = false;
        }

        internal FTPControlSocket(string remoteHost, int controlPort, int timeout, Encoding encoding)
        {
            this.log = Logger.GetLogger("FTPControlSocket");
            this.synchronizePassiveConnections = false;
            this.strictReturnCodes = false;
            this.remoteHost = null;
            this.remoteAddr = null;
            this.controlPort = -1;
            this.controlSock = null;
            this.timeout = 0;
            this.writer = null;
            this.reader = null;
            this.activePortRange = null;
            this.activeIPAddress = null;
            this.nextPort = 0;
            this.autoPassiveIPSubstitution = false;
            if (this.activePortRange != null)
            {
                this.activePortRange.ValidateRange();
            }
            this.Initialize(new StandardSocket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp), remoteHost, controlPort, timeout, encoding);
        }

        internal virtual void ConnectSocket(BaseSocket socket, string address, int port)
        {
            this.remoteAddr = HostNameResolver.GetAddress(address);
            socket.Connect(new IPEndPoint(this.remoteAddr, port));
        }

        internal virtual FTPDataSocket CreateDataSocket(FTPConnectMode connectMode)
        {
            if (connectMode == FTPConnectMode.ACTIVE)
            {
                return this.CreateDataSocketActive();
            }
            return this.CreateDataSocketPASV();
        }

        internal virtual FTPDataSocket CreateDataSocketActive()
        {
            FTPDataSocket socket;
            try
            {
                int num = 0;
                int num2 = 100;
                if (this.activePortRange != null)
                {
                    int num3 = (this.activePortRange.HighPort - this.activePortRange.LowPort) + 1;
                    if (num3 < 100)
                    {
                        num2 = num3;
                    }
                }
                while (num < num2)
                {
                    num++;
                    try
                    {
                        return this.NewActiveDataSocket(this.nextPort);
                    }
                    catch (SocketException)
                    {
                        if (num < num2)
                        {
                            this.log.Warn("Detected socket in use - retrying and selecting new port");
                            this.SetNextAvailablePortFromRange();
                        }
                        continue;
                    }
                }
                throw new FTPException("Exhausted active port retry count - giving up");
            }
            finally
            {
                this.SetNextAvailablePortFromRange();
            }
            return socket;
        }

        internal virtual FTPDataSocket CreateDataSocketPASV()
        {
            FTPDataSocket socket;
            bool synchronizePassiveConnections = this.SynchronizePassiveConnections;
            Mutex mutex = null;
            if (synchronizePassiveConnections)
            {
                mutex = new Mutex(false, PASV_MUTEX_NAME);
                mutex.WaitOne();
            }
            try
            {
                FTPReply reply = this.SendCommand("PASV");
                this.ValidateReply(reply, new string[] { "227" });
                string replyText = reply.ReplyText;
                Match match = new Regex(@"(?<a0>\d{1,3}),(?<a1>\d{1,3}),(?<a2>\d{1,3}),(?<a3>\d{1,3}),(?<p0>\d{1,3}),(?<p1>\d{1,3})").Match(replyText);
                string str2 = match.Groups["a0"].Value + "." + match.Groups["a1"].Value + "." + match.Groups["a2"].Value + "." + match.Groups["a3"].Value;
                this.log.Debug("Server supplied address=" + str2);
                int[] numArray = new int[] { int.Parse(match.Groups["p0"].Value), int.Parse(match.Groups["p1"].Value) };
                int port = (numArray[0] << 8) + numArray[1];
                this.log.Debug("Server supplied port=" + port);
                string str3 = str2;
                if (this.autoPassiveIPSubstitution && (this.remoteAddr != null))
                {
                    str3 = this.remoteAddr.ToString();
                    if (this.log.IsEnabledFor(Level.DEBUG))
                    {
                        this.log.Debug(string.Format("Substituting server supplied IP ({0}) with remote host IP ({1})", str2, str3));
                    }
                }
                socket = this.NewPassiveDataSocket(str3, port);
            }
            finally
            {
                if (synchronizePassiveConnections && (mutex != null))
                {
                    mutex.ReleaseMutex();
                    mutex.Close();
                }
            }
            return socket;
        }

        internal void Initialize(BaseSocket sock, string remoteHost, int controlPort, int timeout, Encoding encoding)
        {
            this.remoteHost = remoteHost;
            this.controlPort = controlPort;
            this.timeout = timeout;
            this.controlSock = sock;
            this.ConnectSocket(this.controlSock, remoteHost, controlPort);
            this.Timeout = timeout;
            this.InitStreams(encoding);
        }

        internal void InitStreams(Encoding encoding)
        {
            Stream stream = this.controlSock.GetStream();
            if (encoding == null)
            {
                encoding = Encoding.ASCII;
            }
            this.encoding = encoding;
            this.log.Debug("Command encoding=" + encoding.ToString());
            this.writer = new StreamWriter(stream, encoding);
            this.reader = new StreamReader(stream, encoding);
        }

        internal void Kill()
        {
            try
            {
                if (this.controlSock != null)
                {
                    this.controlSock.Close();
                }
                this.controlSock = null;
            }
            catch (Exception exception)
            {
                this.log.Debug("Killed socket", exception);
            }
            this.log.Info("Killed control socket");
        }

        internal virtual void Log(string msg, bool command)
        {
            if (msg.StartsWith(PASSWORD_MESSAGE))
            {
                msg = PASSWORD_MESSAGE + " ********";
            }
            this.log.Debug(msg);
            if (command)
            {
                if (this.CommandSent != null)
                {
                    this.CommandSent(this, new FTPMessageEventArgs(msg));
                }
            }
            else if (this.ReplyReceived != null)
            {
                this.ReplyReceived(this, new FTPMessageEventArgs(msg));
            }
        }

        internal virtual void Logout()
        {
            SystemException t = null;
            try
            {
                this.writer.Close();
            }
            catch (SystemException exception2)
            {
                t = exception2;
            }
            try
            {
                this.reader.Close();
            }
            catch (SystemException exception3)
            {
                t = exception3;
            }
            try
            {
                this.controlSock.Close();
                this.controlSock = null;
            }
            catch (SystemException exception4)
            {
                t = exception4;
            }
            if (t != null)
            {
                this.log.Error("Caught exception", t);
                throw t;
            }
        }

        internal virtual FTPDataSocket NewActiveDataSocket(int port)
        {
            BaseSocket sock = new StandardSocket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            try
            {
                this.log.Debug("NewActiveDataSocket(" + port + ")");
                IPEndPoint localEP = new IPEndPoint(((IPEndPoint) this.controlSock.LocalEndPoint).Address, port);
                sock.Bind(localEP);
                sock.Listen(5);
                this.SetDataPort((IPEndPoint) sock.LocalEndPoint);
            }
            catch (Exception exception)
            {
                this.log.Error("Failed to create listening socket", exception);
                sock.Close();
                throw;
            }
            return new FTPActiveDataSocket(sock);
        }

        internal virtual FTPDataSocket NewPassiveDataSocket(string ipAddress, int port)
        {
            this.log.Debug(string.Concat(new object[] { "NewPassiveDataSocket(", ipAddress, ",", port, ")" }));
            IPEndPoint remoteEP = new IPEndPoint(IPAddress.Parse(ipAddress), port);
            BaseSocket sock = new StandardSocket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
            try
            {
                this.SetSocketTimeout(sock, this.timeout);
                sock.Connect(remoteEP);
            }
            catch (Exception exception)
            {
                this.log.Error("Failed to create connecting socket", exception);
                sock.Close();
                throw;
            }
            return new FTPPassiveDataSocket(sock);
        }

        private string ReadLine()
        {
            int num = 0;
            StringBuilder builder = new StringBuilder();
            StringBuilder builder2 = new StringBuilder();
            while (true)
            {
                try
                {
                    num = this.reader.Read();
                }
                catch (IOException exception)
                {
                    this.log.Error("Read failed ('" + builder2.ToString() + "' read so far)");
                    throw new ControlChannelIOException(exception.Message);
                }
                if (num < 0)
                {
                    string message = "Control channel unexpectedly closed ('" + builder2.ToString() + "' read so far)";
                    this.log.Error(message);
                    throw new ControlChannelIOException(message);
                }
                if (num == 10)
                {
                    return builder.ToString();
                }
                if (num != 13)
                {
                    builder.Append((char) num);
                    builder2.Append((char) num);
                }
            }
        }

        internal virtual FTPReply ReadReply()
        {
            string msg = this.ReadLine();
            while (msg.Length == 0)
            {
                msg = this.ReadLine();
            }
            this.Log(msg, false);
            if (msg.Length < 3)
            {
                string message = "Short reply received (" + msg + ")";
                this.log.Error(message);
                throw new MalformedReplyException(message);
            }
            string str3 = msg.Substring(0, 3);
            StringBuilder builder = new StringBuilder("");
            if (msg.Length > 3)
            {
                builder.Append(msg.Substring(4));
            }
            ArrayList list = null;
            if (msg[3] == '-')
            {
                list = ArrayList.Synchronized(new ArrayList(10));
                bool flag = false;
                while (!flag)
                {
                    msg = this.ReadLine();
                    if (msg.Length != 0)
                    {
                        this.Log(msg, false);
                        if (((msg.Length > 3) && msg.Substring(0, 3).Equals(str3)) && (msg[3] == ' '))
                        {
                            builder.Append(msg.Substring(3));
                            flag = true;
                        }
                        else
                        {
                            builder.Append(" ").Append(msg);
                            list.Add(msg);
                        }
                    }
                }
            }
            if (list != null)
            {
                string[] array = new string[list.Count];
                list.CopyTo(array);
                return new FTPReply(str3, builder.ToString(), array);
            }
            return new FTPReply(str3, builder.ToString());
        }

        public virtual FTPReply SendCommand(string command)
        {
            FTPReply reply;
            try
            {
                this.WriteCommand(command);
                reply = this.ReadReply();
            }
            catch (Exception exception)
            {
                this.log.Error("Exception in SendCommand", exception);
                if (this.CommandError != null)
                {
                    this.CommandError(this, new FTPErrorEventArgs(exception));
                }
                throw;
            }
            return reply;
        }

        internal void SetActiveIPAddress(IPAddress address)
        {
            this.activeIPAddress = address;
        }

        internal void SetActivePortRange(PortRange portRange)
        {
            portRange.ValidateRange();
            this.activePortRange = portRange;
            if (!portRange.UseOSAssignment)
            {
                this.nextPort = new Random().Next(this.activePortRange.LowPort, this.activePortRange.HighPort);
                this.log.Debug(string.Concat(new object[] { "SetActivePortRange(", this.activePortRange.LowPort, ",", this.activePortRange.HighPort, "). NextPort=", this.nextPort }));
            }
        }

        internal void SetDataPort(IPEndPoint ep)
        {
            byte[] bytes = BitConverter.GetBytes(ep.Address.Address);
            if (this.activeIPAddress != null)
            {
                this.log.Info("Forcing use of fixed IP for PORT command");
                bytes = BitConverter.GetBytes(this.activeIPAddress.Address);
            }
            byte[] buffer2 = this.ToByteArray((ushort) ep.Port);
            string command = new StringBuilder("PORT ").Append((short) bytes[0]).Append(",").Append((short) bytes[1]).Append(",").Append((short) bytes[2]).Append(",").Append((short) bytes[3]).Append(",").Append((short) buffer2[0]).Append(",").Append((short) buffer2[1]).ToString();
            FTPReply reply = this.SendCommand(command);
            this.ValidateReply(reply, new string[] { "200", "250" });
        }

        private void SetNextAvailablePortFromRange()
        {
            if ((this.activePortRange != null) && !this.activePortRange.UseOSAssignment)
            {
                if (this.nextPort == 0)
                {
                    this.nextPort = new Random().Next(this.activePortRange.LowPort, this.activePortRange.HighPort);
                }
                else
                {
                    this.nextPort++;
                }
                if (this.nextPort > this.activePortRange.HighPort)
                {
                    this.nextPort = this.activePortRange.LowPort;
                }
                this.log.Debug("Next active port will be: " + this.nextPort);
            }
        }

        internal void SetSocketTimeout(BaseSocket sock, int timeout)
        {
            if (timeout > 0)
            {
                try
                {
                    sock.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReceiveTimeout, timeout);
                    sock.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.SendTimeout, timeout);
                }
                catch (SocketException exception)
                {
                    this.log.Warn("Failed to set socket timeout: " + exception.Message);
                }
            }
        }

        internal byte[] ToByteArray(ushort val)
        {
            return new byte[] { ((byte) (val >> 8)), ((byte) (val & 0xff)) };
        }

        internal void ValidateConnection()
        {
            FTPReply reply = this.ReadReply();
            this.ValidateReply(reply, new string[] { "220", "230" });
        }

        public virtual FTPReply ValidateReply(FTPReply reply, params string[] expectedReplyCodes)
        {
            if ("421" == reply.ReplyCode)
            {
                this.log.Error("Received 421 - throwing exception");
                throw new FTPConnectionClosedException(reply.ReplyText);
            }
            foreach (string str in expectedReplyCodes)
            {
                if (this.strictReturnCodes)
                {
                    if (reply.ReplyCode == str)
                    {
                        return reply;
                    }
                }
                else if (reply.ReplyCode[0] == str[0])
                {
                    return reply;
                }
            }
            StringBuilder builder = new StringBuilder("[");
            int num2 = 0;
            foreach (string str2 in expectedReplyCodes)
            {
                builder.Append(str2);
                if ((num2 + 1) < expectedReplyCodes.Length)
                {
                    builder.Append(",");
                }
                num2++;
            }
            builder.Append("]");
            this.log.Info(string.Concat(new object[] { "Expected reply codes = ", builder.ToString(), " (strict=", this.strictReturnCodes, ")" }));
            throw new FTPException(reply);
        }

        internal virtual void WriteCommand(string command)
        {
            this.Log("---> " + command, true);
            this.writer.Write(command + "\r\n");
            this.writer.Flush();
        }

        internal bool AutoPassiveIPSubstitution
        {
            get
            {
                return this.autoPassiveIPSubstitution;
            }
            set
            {
                this.autoPassiveIPSubstitution = value;
            }
        }

        public bool Connected
        {
            get
            {
                return ((this.controlSock != null) && this.controlSock.Connected);
            }
        }

        internal IPAddress LocalAddress
        {
            get
            {
                return ((IPEndPoint) this.controlSock.LocalEndPoint).Address;
            }
        }

        internal virtual bool StrictReturnCodes
        {
            get
            {
                return this.strictReturnCodes;
            }
            set
            {
                this.log.Debug("StrictReturnCodes=" + value);
                this.strictReturnCodes = value;
            }
        }

        internal virtual bool SynchronizePassiveConnections
        {
            get
            {
                return this.synchronizePassiveConnections;
            }
            set
            {
                this.synchronizePassiveConnections = value;
            }
        }

        internal virtual int Timeout
        {
            get
            {
                return this.timeout;
            }
            set
            {
                this.timeout = value;
                this.log.Debug("Setting socket timeout=" + value);
                if (this.controlSock == null)
                {
                    throw new SystemException("Failed to set timeout - no control socket");
                }
                this.SetSocketTimeout(this.controlSock, value);
            }
        }
    }
}

