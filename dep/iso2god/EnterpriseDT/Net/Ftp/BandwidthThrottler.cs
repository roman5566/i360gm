namespace EnterpriseDT.Net.Ftp
{
    using EnterpriseDT.Util.Debug;
    using System;
    using System.Threading;

    public class BandwidthThrottler
    {
        private long lastBytes = 0L;
        private DateTime lastTime = DateTime.MinValue;
        private Logger log = Logger.GetLogger("BandwidthThrottler");
        private int thresholdBytesPerSec = -1;

        public BandwidthThrottler(int thresholdBytesPerSec)
        {
            this.thresholdBytesPerSec = thresholdBytesPerSec;
        }

        public void Reset()
        {
            this.lastTime = DateTime.Now;
            this.lastBytes = 0L;
        }

        public void ThrottleTransfer(long bytesSoFar)
        {
            DateTime now = DateTime.Now;
            long num = bytesSoFar - this.lastBytes;
            TimeSpan span = (TimeSpan) (now - this.lastTime);
            long totalMilliseconds = (long) span.TotalMilliseconds;
            if (totalMilliseconds != 0L)
            {
                double num3 = (((double) num) / ((double) totalMilliseconds)) * 1000.0;
                this.log.Debug("rate={0}", new object[] { num3 });
                while (num3 > this.thresholdBytesPerSec)
                {
                    this.log.Debug("Sleeping to decrease transfer rate (rate = {0} bytes/s)", new object[] { num3 });
                    Thread.Sleep(100);
                    span = (TimeSpan) (DateTime.Now - this.lastTime);
                    totalMilliseconds = (long) span.TotalMilliseconds;
                    num3 = (((double) num) / ((double) totalMilliseconds)) * 1000.0;
                }
                this.lastTime = now;
                this.lastBytes = bytesSoFar;
            }
        }

        public int Threshold
        {
            get
            {
                return this.thresholdBytesPerSec;
            }
            set
            {
                this.thresholdBytesPerSec = value;
            }
        }
    }
}

