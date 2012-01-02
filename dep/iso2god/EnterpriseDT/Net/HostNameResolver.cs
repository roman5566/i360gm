namespace EnterpriseDT.Net
{
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Net;
    using System.Text.RegularExpressions;

    internal class HostNameResolver
    {
        private const string IP_ADDRESS_REGEX = @"[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}";
        private static Logger log = Logger.GetLogger("HostNameResolver");

        public static IPAddress GetAddress(string hostName)
        {
            if (hostName == null)
            {
                throw new ArgumentNullException();
            }
            IPAddress address = null;
            if (Regex.IsMatch(hostName, @"[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}"))
            {
                address = IPAddress.Parse(hostName);
            }
            else
            {
                address = Dns.Resolve(hostName).AddressList[0];
            }
            if (log.DebugEnabled)
            {
                log.Debug(hostName + " resolved to " + address.ToString());
            }
            return address;
        }
    }
}

