namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.ComponentModel;

    public class PortRange
    {
        internal const int DEFAULT_HIGH_PORT = 0x1388;
        private int high;
        private const int HIGH_PORT = 0xffff;
        private int low;
        internal const int LOW_PORT = 0x400;
        private PropertyChangedEventHandler propertyChangeHandler;

        internal PortRange()
        {
            this.propertyChangeHandler = null;
            this.low = 0x400;
            this.high = 0x1388;
        }

        internal PortRange(int low, int high)
        {
            this.propertyChangeHandler = null;
            if ((low < 0x400) || (high > 0xffff))
            {
                throw new ArgumentException(string.Concat(new object[] { "Ports must be in range [", 0x400, ",", 0xffff, "]" }));
            }
            if (low >= high)
            {
                throw new ArgumentException(string.Concat(new object[] { "Low port (", low, ") must be smaller than high port (", high, ")" }));
            }
            this.low = low;
            this.high = high;
        }

        public override string ToString()
        {
            return string.Format("{0} -> {1}", this.low, this.high);
        }

        internal void ValidateRange()
        {
            if (this.low >= this.high)
            {
                throw new FTPException(string.Concat(new object[] { "Low port (", this.low, ") must be smaller than high port (", this.high, ")" }));
            }
        }

        [DefaultValue(0x1388), Description("Highest port number in range."), RefreshProperties(RefreshProperties.All)]
        public int HighPort
        {
            get
            {
                return this.high;
            }
            set
            {
                if ((value > 0xffff) || (value < 0x400))
                {
                    throw new ArgumentException(string.Concat(new object[] { "Ports must be in range [", 0x400, ",", 0xffff, "]" }));
                }
                if (this.HighPort != value)
                {
                    this.high = value;
                    if (this.propertyChangeHandler != null)
                    {
                        this.propertyChangeHandler(this, new PropertyChangedEventArgs("HighPort"));
                    }
                }
            }
        }

        [RefreshProperties(RefreshProperties.All), DefaultValue(0x400), Description("Lowest port number in range.")]
        public int LowPort
        {
            get
            {
                return this.low;
            }
            set
            {
                if ((value > 0xffff) || (value < 0x400))
                {
                    throw new ArgumentException(string.Concat(new object[] { "Ports must be in range [", 0x400, ",", 0xffff, "]" }));
                }
                if (this.LowPort != value)
                {
                    this.low = value;
                    if (this.propertyChangeHandler != null)
                    {
                        this.propertyChangeHandler(this, new PropertyChangedEventArgs("LowPort"));
                    }
                }
            }
        }

        internal PropertyChangedEventHandler PropertyChangeHandler
        {
            get
            {
                return this.propertyChangeHandler;
            }
            set
            {
                this.propertyChangeHandler = value;
            }
        }

        [DefaultValue(true), RefreshProperties(RefreshProperties.All), Description("Determines if the operating system should select the ports within the range 1024-5000.")]
        public bool UseOSAssignment
        {
            get
            {
                return ((this.low == 0x400) && (this.high == 0x1388));
            }
            set
            {
                this.LowPort = 0x400;
                this.HighPort = 0x1388;
            }
        }
    }
}

