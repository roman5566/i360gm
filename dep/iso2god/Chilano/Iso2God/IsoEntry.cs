namespace Chilano.Iso2God
{
    using System;
    using System.Runtime.InteropServices;

    [StructLayout(LayoutKind.Sequential)]
    public struct IsoEntry
    {
        public IsoEntryPlatform Platform;
        public string Path;
        public string Destination;
        public string TitleName;
        public long Size;
        public uint Parts;
        public byte[] Thumb;
        public IsoEntryID ID;
        public IsoEntryStatus Status;
        public IsoEntryPadding Padding;
        public IsoEntry(IsoEntryPlatform Platform, string Path, string Destination, long Size, string TitleName, IsoEntryID ID, byte[] Thumb, IsoEntryPadding Padding)
        {
            this.Platform = Platform;
            this.Path = Path;
            this.Destination = Destination;
            this.Size = Size;
            this.TitleName = TitleName;
            this.Parts = 0;
            this.ID = ID;
            this.Status = IsoEntryStatus.Idle;
            this.Thumb = Thumb;
            this.Padding = Padding;
        }
    }
}

