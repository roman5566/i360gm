namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.Threading;

    internal class FTPSemaphore
    {
        private long count = 0L;
        private object syncLock = new object();

        internal FTPSemaphore(int initCount)
        {
            this.count = initCount;
        }

        internal void Release()
        {
            lock (this.syncLock)
            {
                this.count += 1L;
                Monitor.Pulse(this.syncLock);
            }
        }

        internal void WaitOne(int timeoutMillis)
        {
            lock (this.syncLock)
            {
                while (this.count == 0L)
                {
                    Monitor.Wait(this.syncLock, timeoutMillis);
                }
                this.count -= 1L;
            }
        }
    }
}

