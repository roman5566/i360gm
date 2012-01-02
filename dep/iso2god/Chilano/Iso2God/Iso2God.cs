namespace Chilano.Iso2God
{
    using Chilano.Iso2God.ConHeader;
    using Chilano.Iso2God.ConStructures;
    using Chilano.Xbox360.IO;
    using Chilano.Xbox360.Iso;
    using System;
    using System.ComponentModel;
    using System.IO;
    using System.Runtime.CompilerServices;
    using System.Runtime.InteropServices;
    using System.Security.Cryptography;

    public class Iso2God : BackgroundWorker
    {
        private uint blockPerPart = 0xa1c4;
        private uint blockPerSHT = 0xcc;
        private uint blockSize = 0x1000;
        private DateTime Finish;
        private uint freeSector = 0x24;
        private static byte[] gdf_sector = new byte[] { 
            1, 0x43, 0x44, 0x30, 0x30, 0x31, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0x17, 0x4b, 0, 0, 0, 0, 0x4b, 0x17, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 
            0, 8, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
            0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x30, 0x30, 0x30, 
            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0, 0x30, 0x30, 
            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0, 0x30, 
            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0, 
            0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 
            0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
            0xff, 0x43, 0x44, 0x30, 0x30, 0x31, 1
         };
        private float progress;
        private GDFDirTable rootDir;
        private SHA1Managed sha1 = new SHA1Managed();
        private uint shtPerMHT = 0xcb;
        private DateTime Start;
        private string uniqueName = "";

        public event Iso2GodCompletedEventHandler Completed;

        public event Iso2GodProgressEventHandler Progress;

        public Iso2God()
        {
            base.WorkerReportsProgress = true;
            base.WorkerSupportsCancellation = false;
            base.ProgressChanged += new ProgressChangedEventHandler(this.Iso2God_ProgressChanged);
            base.RunWorkerCompleted += new RunWorkerCompletedEventHandler(this.Iso2God_RunWorkerCompleted);
            base.DoWork += new DoWorkEventHandler(this.Iso2God_Run);
        }

        private void calcMhtHashChain(string Destination, uint TotalPartsReq, out uint lastPartSize, out byte[] lastMhtHash)
        {
            lastPartSize = 0;
            lastMhtHash = new byte[20];
            for (uint i = TotalPartsReq - 1; i > 0; i--)
            {
                string path = Destination + Path.DirectorySeparatorChar + "Data";
                if (i < 10)
                {
                    path = path + "000" + i.ToString();
                }
                else if (i < 100)
                {
                    path = path + "00" + i.ToString();
                }
                else if (i < 0x3e8)
                {
                    path = path + "0" + i.ToString();
                }
                else if (i < 0x2710)
                {
                    path = path + i.ToString();
                }
                string str2 = Destination + Path.DirectorySeparatorChar + "Data";
                if ((i - 1) < 10)
                {
                    str2 = str2 + "000" + ((i - 1)).ToString();
                }
                else if ((i - 1) < 100)
                {
                    str2 = str2 + "00" + ((i - 1)).ToString();
                }
                else if ((i - 1) < 0x3e8)
                {
                    str2 = str2 + "0" + ((i - 1)).ToString();
                }
                else if ((i - 1) < 0x2710)
                {
                    str2 = str2 + ((i - 1)).ToString();
                }
                FileStream f = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.None);
                FileStream stream2 = new FileStream(str2, FileMode.Open, FileAccess.ReadWrite, FileShare.None);
                if (i == (TotalPartsReq - 1))
                {
                    lastPartSize = (uint) f.Length;
                }
                MasterHashtable hashtable = new MasterHashtable();
                hashtable.ReadMHT(f);
                byte[] array = new byte[this.blockSize];
                hashtable.ToByteArray().CopyTo(array, 0);
                byte[] item = new byte[20];
                item = this.sha1.ComputeHash(array);
                MasterHashtable hashtable2 = new MasterHashtable();
                hashtable2.ReadMHT(stream2);
                hashtable2.Add(item);
                stream2.Seek(0L, SeekOrigin.Begin);
                hashtable2.Write(stream2);
                if ((i - 1) == 0)
                {
                    lastMhtHash = this.sha1.ComputeHash(hashtable2.ToByteArray());
                }
                f.Close();
                stream2.Close();
            }
        }

        private void calcPath(GDFDirTable t, GDFDirEntry e, ref string path)
        {
            if (e != null)
            {
                path = path.Insert(0, @"\" + e.Name);
                if (e.Parent != null)
                {
                    this.calcPath(e.Parent, null, ref path);
                }
            }
            else if (t.Parent != null)
            {
                path = path.Insert(0, @"\" + t.Parent.Name);
                if (t.Parent.Parent != null)
                {
                    this.calcPath(t.Parent.Parent, null, ref path);
                }
            }
        }

        private void createConHeader(string path, IsoEntry iso, uint blocksAllocated, ushort blocksNotAllocated, uint totalParts, ulong sizeParts, byte[] mhtHash)
        {
            ConHeaderWriter writer = new ConHeaderWriter();
            writer.WriteIDs(iso.ID.TitleID, iso.ID.MediaID, iso.TitleName);
            writer.WriteExecutionDetails(iso.ID.DiscNumber, iso.ID.DiscCount, iso.ID.Platform, iso.ID.ExType);
            writer.WriteBlockCounts(blocksAllocated, blocksNotAllocated);
            writer.WriteDataPartsInfo(totalParts, sizeParts);
            writer.WriteGameIcon(iso.Thumb);
            switch (iso.Platform)
            {
                case IsoEntryPlatform.Xbox:
                    writer.WriteContentType(ContentType.XboxOriginal);
                    break;

                case IsoEntryPlatform.Xbox360:
                    writer.WriteContentType(ContentType.GamesOnDemand);
                    break;
            }
            writer.WriteMhtHash(mhtHash);
            writer.WriteHash();
            writer.Write(path);
        }

        private string createUniqueName(IsoEntry Iso)
        {
            MemoryStream s = new MemoryStream();
            CBinaryWriter writer = new CBinaryWriter(EndianType.LittleEndian, s);
            writer.Write(Iso.ID.TitleID);
            writer.Write(Iso.ID.MediaID);
            writer.Write(Iso.ID.DiscNumber);
            writer.Write(Iso.ID.DiscCount);
            byte[] buffer = this.sha1.ComputeHash(s.ToArray());
            string str = "";
            for (int i = 0; i < (buffer.Length / 2); i++)
            {
                str = str + buffer[i].ToString("X02");
            }
            return str;
        }

        private void Iso2God_Full(object sender, DoWorkEventArgs e)
        {
            FileStream stream;
            FileStream stream2;
            base.ReportProgress((int) this.progress, "Preparing to rebuild ISO image...");
            IsoEntry argument = (IsoEntry) e.Argument;
            try
            {
                stream = new FileStream(argument.Path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            }
            catch (Exception exception)
            {
                base.ReportProgress(0, "Error! " + exception.Message);
                return;
            }
            using (GDF gdf = new GDF(stream))
            {
                uint lastSector = 0;
                gdf.ParseDirectory(gdf.RootDir, true, ref lastSector);
                base.ReportProgress((int) this.progress, "Generating new GDFS structures...");
                try
                {
                    stream2 = File.OpenWrite(argument.Padding.IsoPath + argument.Path.Substring(argument.Path.LastIndexOf(Path.DirectorySeparatorChar) + 1) + "_rebuilt.iso");
                }
                catch (Exception exception2)
                {
                    base.ReportProgress(0, "Error rebuilding GDF! " + exception2.Message);
                    return;
                }
                this.rootDir = (GDFDirTable) gdf.RootDir.Clone();
                this.RemapSectors(gdf);
                this.WriteGDF(gdf, stream2);
                this.WriteFiles(gdf, stream2);
            }
            if (stream2.Length > 0L)
            {
                base.ReportProgress((int) this.progress, "ISO image rebuilt.");
                argument.Path = stream2.Name;
                argument.Size = stream2.Length;
                stream2.Close();
                stream.Close();
                this.Iso2God_Partial(sender, e, false, argument);
            }
            else
            {
                base.ReportProgress(100, "Failed to rebuild ISO. Aborting.");
            }
        }

        private void Iso2God_Partial(object sender, DoWorkEventArgs e, bool Crop, IsoEntry iso)
        {
            FileStream stream;
            GDF gdf;
            base.ReportProgress((int) this.progress, "Examining ISO image...");
            try
            {
                stream = new FileStream(iso.Path, FileMode.Open, FileAccess.Read, FileShare.Read);
            }
            catch (Exception)
            {
                base.ReportProgress(0, "Cannot access the ISO image because it is being accessed by another application.");
                return;
            }
            try
            {
                gdf = new GDF(stream);
            }
            catch (Exception exception)
            {
                base.ReportProgress(0, "Error while parsing GDF: " + exception.Message);
                return;
            }
            ulong num = 0L;
            if (Crop)
            {
                num = (((ulong) iso.Size) - gdf.RootOffset) - (((ulong) iso.Size) - (gdf.LastOffset + gdf.RootOffset));
            }
            else
            {
                num = ((ulong) iso.Size) - gdf.RootOffset;
            }
            uint blocksReq = (uint) Math.Ceiling((double) (((double) num) / ((double) this.blockSize)));
            uint partsReq = (uint) Math.Ceiling((double) (((double) blocksReq) / ((double) this.blockPerPart)));
            ContentType type = (iso.Platform == IsoEntryPlatform.Xbox360) ? ContentType.GamesOnDemand : ContentType.XboxOriginal;
            object[] objArray = new object[] { iso.Destination, iso.ID.TitleID, Path.DirectorySeparatorChar, "0000", ((uint) type).ToString("X02"), Path.DirectorySeparatorChar };
            string path = string.Concat(objArray) + ((this.uniqueName != null) ? this.uniqueName : iso.ID.TitleID) + ".data";
            if (Directory.Exists(path))
            {
                Directory.Delete(path, true);
            }
            Directory.CreateDirectory(path);
            base.ReportProgress((int) this.progress, "Beginning ISO conversion...");
            stream.Seek((long) gdf.RootOffset, SeekOrigin.Begin);
            this.writeParts(stream, path, iso, partsReq, blocksReq);
            base.ReportProgress((int) this.progress, "Calculating Master Hash Table chain...");
            byte[] lastMhtHash = new byte[20];
            uint lastPartSize = 0;
            this.calcMhtHashChain(path, partsReq, out lastPartSize, out lastMhtHash);
            ulong num6 = 0xa290L;
            ulong num7 = this.blockSize * num6;
            ulong sizeParts = lastPartSize + ((partsReq - 1) * num7);
            base.ReportProgress(0x5f, "Creating LIVE header...");
            this.createConHeader(path.Substring(0, path.Length - 5), iso, blocksReq, 0, partsReq, sizeParts, lastMhtHash);
            stream.Close();
            stream.Dispose();
            gdf.Dispose();
            if ((iso.Padding.Type == IsoEntryPaddingRemoval.Full) && !iso.Padding.KeepIso)
            {
                try
                {
                    File.Delete(iso.Path);
                }
                catch (Exception)
                {
                    base.ReportProgress(0x5f, "Unable to delete ISO temporary image.");
                }
            }
            this.Finish = DateTime.Now;
            TimeSpan span = (TimeSpan) (this.Finish - this.Start);
            base.ReportProgress(100, "Done!");
            e.Result = "Finished in " + span.Minutes.ToString() + "m" + span.Seconds.ToString() + "s. GOD package written to: " + path;
            GC.Collect();
        }

        private void Iso2God_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            this.OnProgressChange(new Iso2GodProgressArgs(e));
        }

        private void Iso2God_Run(object sender, DoWorkEventArgs e)
        {
            this.Start = DateTime.Now;
            IsoEntry argument = (IsoEntry) e.Argument;
            this.uniqueName = this.createUniqueName(argument);
            switch (argument.Padding.Type)
            {
                case IsoEntryPaddingRemoval.None:
                    this.Iso2God_Partial(sender, e, false, argument);
                    return;

                case IsoEntryPaddingRemoval.Partial:
                    this.Iso2God_Partial(sender, e, true, argument);
                    return;

                case IsoEntryPaddingRemoval.Full:
                    this.Iso2God_Full(sender, e);
                    return;
            }
        }

        private void Iso2God_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            this.OnCompleted(new Iso2GodCompletedArgs(e, this.uniqueName));
        }

        protected virtual void OnCompleted(Iso2GodCompletedArgs e)
        {
            if (this.Completed != null)
            {
                this.Completed(this, e);
            }
        }

        protected virtual void OnProgressChange(Iso2GodProgressArgs e)
        {
            if (this.Progress != null)
            {
                this.Progress(this, e);
            }
        }

        private void remapDirs(GDF src, GDFDirTable table)
        {
            foreach (GDFDirEntry entry in table)
            {
                if (entry.IsDirectory)
                {
                    if (entry.SubDir == null)
                    {
                        entry.Sector = 0;
                        entry.Size = 0;
                    }
                    else
                    {
                        entry.Sector = this.freeSector;
                        entry.SubDir.Sector = this.freeSector;
                        entry.SubDir.Parent = entry;
                        this.freeSector += this.sizeToSectors(src, entry.Size);
                        base.ReportProgress(0, "Remapped '" + entry.Name + "' (" + this.sizeToSectors(src, entry.Size).ToString() + " sectors) to Sector 0x" + entry.Sector.ToString("X02"));
                        this.remapDirs(src, entry.SubDir);
                    }
                }
            }
        }

        private void remapFiles(GDF src, GDFDirTable table)
        {
            foreach (GDFDirEntry entry in table)
            {
                if (!entry.IsDirectory)
                {
                    entry.Sector = this.freeSector;
                    this.freeSector += this.sizeToSectors(src, entry.Size);
                }
            }
            foreach (GDFDirEntry entry2 in table)
            {
                if (entry2.IsDirectory && (entry2.SubDir != null))
                {
                    this.remapFiles(src, entry2.SubDir);
                }
            }
        }

        public void RemapSectors(GDF src)
        {
            if (this.rootDir != null)
            {
                this.rootDir.Sector = this.freeSector;
                this.rootDir.Size = (uint) this.sectorsToSize(src, this.sizeToSectors(src, this.rootDir.Size));
                this.freeSector += this.sizeToSectors(src, this.rootDir.Size);
                this.remapDirs(src, this.rootDir);
                this.remapFiles(src, this.rootDir);
            }
        }

        private long sectorsToSize(GDF src, uint sectors)
        {
            return (long) (sectors * src.VolDesc.SectorSize);
        }

        private uint sizeToSectors(GDF src, uint size)
        {
            return (uint) Math.Ceiling((double) (((double) size) / ((double) src.VolDesc.SectorSize)));
        }

        private void writeFiles(GDF src, CBinaryWriter bw, GDFDirTable table)
        {
            uint fileCount = src.FileCount;
            foreach (GDFDirEntry entry in table)
            {
                if (!entry.IsDirectory)
                {
                    bw.Seek((long) (entry.Sector * src.VolDesc.SectorSize), SeekOrigin.Begin);
                    string path = "";
                    this.calcPath(table, entry, ref path);
                    if (path.StartsWith(@"\"))
                    {
                        path = path.Remove(0, 1);
                    }
                    this.progress += ((1f / ((float) fileCount)) * 0.45f) * 100f;
                    base.ReportProgress((int) this.progress, "Writing '" + path + "' at Sector 0x" + entry.Sector.ToString("X02") + "...");
                    src.WriteFileToStream(path, bw);
                }
            }
            foreach (GDFDirEntry entry2 in table)
            {
                if (entry2.IsDirectory && (entry2.SubDir != null))
                {
                    this.writeFiles(src, bw, entry2.SubDir);
                }
            }
        }

        public void WriteFiles(GDF src, FileStream Iso)
        {
            CBinaryWriter bw = new CBinaryWriter(EndianType.LittleEndian, Iso);
            base.ReportProgress(0, "Writing file data to new ISO image...");
            this.writeFiles(src, bw, this.rootDir);
            this.writeGDFsizes(bw);
        }

        public void WriteGDF(GDF src, FileStream iso)
        {
            CBinaryWriter bw = new CBinaryWriter(EndianType.LittleEndian, iso);
            base.ReportProgress(0, "Writing new GDF header...");
            this.writeGDFheader(src, bw);
            base.ReportProgress(0, "Writing new GDF directories...");
            this.writeGDFtable(src, bw, this.rootDir);
        }

        private void writeGDFheader(GDF src, CBinaryWriter bw)
        {
            bw.Seek(0L, SeekOrigin.Begin);
            bw.Write((uint) 0x1a465358);
            bw.Write((uint) 0x400);
            bw.Seek(0x8000L, SeekOrigin.Begin);
            bw.Write(gdf_sector);
            bw.Seek(0x10000L, SeekOrigin.Begin);
            bw.Write(src.VolDesc.Identifier);
            bw.Write(this.rootDir.Sector);
            bw.Write((uint) (this.sizeToSectors(src, this.rootDir.Size) * src.VolDesc.SectorSize));
            bw.Write(src.VolDesc.ImageCreationTime);
            bw.Write((byte) 1);
            bw.Seek(0x107ecL, SeekOrigin.Begin);
            bw.Write(src.VolDesc.Identifier);
        }

        private void writeGDFsizes(CBinaryWriter bw)
        {
            long length = bw.BaseStream.Length;
            bw.Seek(8L, SeekOrigin.Begin);
            bw.Write((long) (length - 0x400L));
            uint num2 = (uint) (((double) length) / 2048.0);
            bw.Seek(0x8050L, SeekOrigin.Begin);
            bw.Write(num2);
            bw.Endian = EndianType.BigEndian;
            bw.Seek(0x8054L, SeekOrigin.Begin);
            bw.Write(num2);
            bw.Endian = EndianType.LittleEndian;
        }

        private void writeGDFtable(GDF src, CBinaryWriter bw, GDFDirTable table)
        {
            bw.Seek((long) (table.Sector * src.VolDesc.SectorSize), SeekOrigin.Begin);
            byte[] buffer = table.ToByteArray();
            bw.Write(buffer);
            foreach (GDFDirEntry entry in table)
            {
                if (entry.IsDirectory && (entry.SubDir != null))
                {
                    this.writeGDFtable(src, bw, entry.SubDir);
                }
            }
        }

        private void writeParts(FileStream src, string destPath, IsoEntry iso, uint partsReq, uint blocksReq)
        {
            uint num = 0;
            for (uint i = 0; i < partsReq; i++)
            {
                this.progress += ((1f / ((float) partsReq)) * ((iso.Padding.Type == IsoEntryPaddingRemoval.Full) ? 0.45f : 0.9f)) * 100f;
                base.ReportProgress((int) this.progress, "Writing Part " + i.ToString() + " / " + partsReq.ToString() + "...");
                string path = destPath + Path.DirectorySeparatorChar + "Data";
                if (i < 10)
                {
                    path = path + "000" + i.ToString();
                }
                else if (i < 100)
                {
                    path = path + "00" + i.ToString();
                }
                else if (i < 0x3e8)
                {
                    path = path + "0" + i.ToString();
                }
                else if (i < 0x2710)
                {
                    path = path + i.ToString();
                }
                FileStream f = new FileStream(path, FileMode.Create, FileAccess.ReadWrite, FileShare.None);
                MasterHashtable hashtable = new MasterHashtable();
                hashtable.WriteBlank(f);
                for (int j = 0; j < this.shtPerMHT; j++)
                {
                    SubHashTable table = new SubHashTable();
                    table.WriteBlank(f);
                    uint num4 = 0;
                    while ((num < blocksReq) && (num4 < this.blockPerSHT))
                    {
                        byte[] buffer = new byte[this.blockSize];
                        src.Read(buffer, 0, buffer.Length);
                        byte[] buffer2 = new byte[20];
                        buffer2 = this.sha1.ComputeHash(buffer, 0, (int) this.blockSize);
                        table.Add(buffer2);
                        f.Write(buffer, 0, buffer.Length);
                        num++;
                        num4++;
                    }
                    long position = f.Position;
                    f.Seek((long) -((ulong) ((num4 + 1) * this.blockSize)), SeekOrigin.Current);
                    table.Write(f);
                    f.Seek(position, SeekOrigin.Begin);
                    byte[] item = new byte[20];
                    item = this.sha1.ComputeHash(table.ToByteArray(), 0, (int) this.blockSize);
                    hashtable.Add(item);
                    if (num >= blocksReq)
                    {
                        break;
                    }
                }
                f.Seek(0L, SeekOrigin.Begin);
                hashtable.Write(f);
                f.Close();
                if (num >= blocksReq)
                {
                    return;
                }
            }
        }
    }
}

