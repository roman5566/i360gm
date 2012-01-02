namespace Chilano.Iso2God
{
    using Chilano.Xbox360.Graphics;
    using Chilano.Xbox360.IO;
    using Chilano.Xbox360.Iso;
    using Chilano.Xbox360.Xbe;
    using Chilano.Xbox360.Xdbf;
    using Chilano.Xbox360.Xex;
    using System;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Imaging;
    using System.IO;
    using System.Text;

    internal class IsoDetails : BackgroundWorker
    {
        private IsoDetailsArgs args;
        private FileStream f;
        private GDF iso;

        public IsoDetails()
        {
            base.WorkerReportsProgress = true;
            base.WorkerSupportsCancellation = false;
            base.DoWork += new DoWorkEventHandler(this.IsoDetails_DoWork);
        }

        private void IsoDetails_DoWork(object sender, DoWorkEventArgs e)
        {
            IsoDetailsPlatform xbox;
            if (e.Argument == null)
            {
                throw new ArgumentNullException("A populated instance of IsoDetailsArgs must be passed.");
            }
            this.args = (IsoDetailsArgs) e.Argument;
            if (this.openIso())
            {
                if (this.iso.Exists("default.xex"))
                {
                    xbox = IsoDetailsPlatform.Xbox360;
                    goto Label_007E;
                }
                if (this.iso.Exists("default.xbe"))
                {
                    xbox = IsoDetailsPlatform.Xbox;
                    goto Label_007E;
                }
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Could not locate default.xex or default.xbe."));
            }
            return;
        Label_007E:
            switch (xbox)
            {
                case IsoDetailsPlatform.Xbox:
                    this.readXbe(e);
                    return;

                case IsoDetailsPlatform.Xbox360:
                    this.readXex(e);
                    return;
            }
        }

        private bool openIso()
        {
            try
            {
                this.f = new FileStream(this.args.PathISO, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
                this.iso = new GDF(this.f);
            }
            catch (IOException exception)
            {
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Failed to open ISO image. Reason:\n\n" + exception.Message));
                return false;
            }
            catch (Exception exception2)
            {
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Unhandled exception occured when opening ISO image. Reason:\n\n" + exception2.Message));
                return false;
            }
            return true;
        }

        private void readXbe(DoWorkEventArgs e)
        {
            IsoDetailsResults results = null;
            byte[] xbe = null;
            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Locating default.xbe..."));
            try
            {
                xbe = this.iso.GetFile("default.xbe");
            }
            catch (Exception exception)
            {
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Unable to extract default.xbe. Reason:\n\n" + exception.Message));
                return;
            }
            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Found! Reading default.xbe..."));
            using (XbeInfo info = new XbeInfo(xbe))
            {
                if (!info.IsValid)
                {
                    base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Default.xbe was not valid."));
                    return;
                }
                results = new IsoDetailsResults(info.Certifcate.TitleName, info.Certifcate.TitleID, (info.Certifcate.DiskNumber > 0) ? info.Certifcate.DiskNumber.ToString() : "1") {
                    DiscCount = "1"
                };
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Extracting thumbnail..."));
                foreach (XbeSection section in info.Sections)
                {
                    if (section.Name == "$$XSIMAGE")
                    {
                        try
                        {
                            XPR xpr = new XPR(section.Data);
                            DDS dds = xpr.ConvertToDDS(0x40, 0x40);
                            Bitmap bitmap = new Bitmap(0x40, 0x40);
                            switch (xpr.Format)
                            {
                                case XPRFormat.ARGB:
                                    bitmap = (Bitmap) dds.GetImage(DDSType.ARGB);
                                    break;

                                case XPRFormat.DXT1:
                                    bitmap = (Bitmap) dds.GetImage(DDSType.DXT1);
                                    break;
                            }
                            MemoryStream stream = new MemoryStream();
                            bitmap.Save(stream, ImageFormat.Png);
                            results.Thumbnail = (Image) bitmap.Clone();
                            results.RawThumbnail = (byte[]) stream.ToArray().Clone();
                            bitmap.Dispose();
                            stream.Dispose();
                            if (xpr.Format == XPRFormat.ARGB)
                            {
                                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "XBE thumbnail type is not supported or is corrupt."));
                            }
                        }
                        catch (Exception exception2)
                        {
                            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Failed to convert thumbnail DDS to PNG.\n\n" + exception2.Message));
                        }
                    }
                }
                if (results.Thumbnail == null)
                {
                    foreach (XbeSection section2 in info.Sections)
                    {
                        if (section2.Name == "$$XTIMAGE")
                        {
                            try
                            {
                                XPR xpr2 = new XPR(section2.Data);
                                DDS dds2 = xpr2.ConvertToDDS(0x80, 0x80);
                                Bitmap bitmap2 = new Bitmap(0x80, 0x80);
                                switch (xpr2.Format)
                                {
                                    case XPRFormat.ARGB:
                                        bitmap2 = (Bitmap) dds2.GetImage(DDSType.ARGB);
                                        break;

                                    case XPRFormat.DXT1:
                                        bitmap2 = (Bitmap) dds2.GetImage(DDSType.DXT1);
                                        break;
                                }
                                Image image = new Bitmap(0x40, 0x40);
                                Graphics graphics = Graphics.FromImage(image);
                                graphics.DrawImage(bitmap2, 0, 0, 0x40, 0x40);
                                MemoryStream stream2 = new MemoryStream();
                                image.Save(stream2, ImageFormat.Png);
                                results.Thumbnail = (Image) image.Clone();
                                results.RawThumbnail = (byte[]) stream2.ToArray().Clone();
                                stream2.Dispose();
                                bitmap2.Dispose();
                                graphics.Dispose();
                                if (xpr2.Format == XPRFormat.ARGB)
                                {
                                    base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "XBE Thumbnail type is not supported or is corrupt."));
                                }
                            }
                            catch (Exception exception3)
                            {
                                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Failed to convert thumbnail DDS to PNG.\n\n" + exception3.Message));
                            }
                        }
                    }
                }
            }
            e.Result = results;
        }

        private void readXex(DoWorkEventArgs e)
        {
            IsoDetailsResults results = null;
            byte[] bytes = null;
            string path = null;
            string pathTemp = null;
            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Locating default.xex..."));
            try
            {
                bytes = this.iso.GetFile("default.xex");
                pathTemp = this.args.PathTemp;
                path = pathTemp + "default.xex";
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Extracting default.xex..."));
                if ((bytes == null) || (bytes.Length == 0))
                {
                    base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Couldn't locate default.xex. Please check this ISO is valid."));
                    return;
                }
                File.WriteAllBytes(path, bytes);
            }
            catch (Exception exception)
            {
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "A problem occured when reading the contents of the ISO image.\n\nPlease ensure this is a valid Xbox 360 ISO by running it through ABGX360.\n\n" + exception.Message));
                return;
            }
            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Found! Reading default.xex..."));
            using (XexInfo info = new XexInfo(bytes))
            {
                if (!info.IsValid)
                {
                    base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Default.xex is not valid."));
                    return;
                }
                if (info.Header.ContainsKey(XexInfoFields.ExecutionInfo))
                {
                    XexExecutionInfo info2 = (XexExecutionInfo) info.Header[XexInfoFields.ExecutionInfo];
                    results = new IsoDetailsResults("", DataConversion.BytesToHexString(info2.TitleID), DataConversion.BytesToHexString(info2.MediaID), info2.Platform.ToString(), info2.ExecutableType.ToString(), info2.DiscNumber.ToString(), info2.DiscCount.ToString(), null);
                }
            }
            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Extracting resources..."));
            Process process = new Process {
                EnableRaisingEvents = false
            };
            process.StartInfo.FileName = this.args.PathXexTool;
            if (File.Exists(process.StartInfo.FileName))
            {
                process.StartInfo.WorkingDirectory = pathTemp;
                process.StartInfo.Arguments = "-d . default.xex";
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.RedirectStandardOutput = false;
                process.StartInfo.CreateNoWindow = true;
                try
                {
                    process.Start();
                    process.WaitForExit();
                    process.Close();
                }
                catch (Win32Exception)
                {
                    base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Could not launch XexTool!"));
                    return;
                }
                if (File.Exists(pathTemp + results.TitleID))
                {
                    Chilano.Xbox360.Xdbf.Xdbf xdbf = new Chilano.Xbox360.Xdbf.Xdbf(File.ReadAllBytes(pathTemp + results.TitleID));
                    base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Progress, "Extracting thumbnail..."));
                    try
                    {
                        byte[] resource = xdbf.GetResource(XdbfResource.Thumb, XdbfResourceType.TitleInfo);
                        MemoryStream stream = new MemoryStream(resource);
                        Image image = Image.FromStream(stream);
                        results.Thumbnail = (Image) image.Clone();
                        results.RawThumbnail = (byte[]) resource.Clone();
                        image.Dispose();
                    }
                    catch (Exception)
                    {
                        try
                        {
                            byte[] buffer = xdbf.GetResource(XdbfResource.Thumb, XdbfResourceType.Achievement);
                            MemoryStream stream2 = new MemoryStream(buffer);
                            Image image2 = Image.FromStream(stream2);
                            results.Thumbnail = (Image) image2.Clone();
                            results.RawThumbnail = (byte[]) buffer.Clone();
                            image2.Dispose();
                        }
                        catch (Exception)
                        {
                            base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Couldn't find thumbnail in XDBF. Possibly corrupt XDBF?"));
                        }
                    }
                    try
                    {
                        MemoryStream stream3 = new MemoryStream(xdbf.GetResource(1, (ushort) 3));
                        stream3.Seek(0x11L, SeekOrigin.Begin);
                        int count = stream3.ReadByte();
                        results.Name = Encoding.UTF8.GetString(stream3.ToArray(), 0x12, count);
                        stream3.Close();
                    }
                    catch (Exception)
                    {
                        try
                        {
                            MemoryStream stream4 = new MemoryStream(xdbf.GetResource(1, (ushort) 0));
                            stream4.Seek(0x11L, SeekOrigin.Begin);
                            int num2 = stream4.ReadByte();
                            results.Name = Encoding.UTF8.GetString(stream4.ToArray(), 0x12, num2);
                            stream4.Close();
                        }
                        catch (Exception)
                        {
                            results.Name = "Unable to read name.";
                        }
                    }
                }
                e.Result = results;
            }
            else
            {
                base.ReportProgress(0, new IsoDetailsResults(IsoDetailsResultsType.Error, "Couldn't locate XexTool. Expected location was:\n" + process.StartInfo.FileName + "\n\nTry disabling User Access Control if it's enabled."));
            }
        }
    }
}

