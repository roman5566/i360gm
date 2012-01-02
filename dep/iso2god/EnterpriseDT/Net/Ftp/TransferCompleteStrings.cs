namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class TransferCompleteStrings : ServerStrings
    {
        private static string TRANSFER_COMPLETE = "TRANSFER COMPLETE";

        public TransferCompleteStrings()
        {
            base.Add(TRANSFER_COMPLETE);
        }
    }
}

