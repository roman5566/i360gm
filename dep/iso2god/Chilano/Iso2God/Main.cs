namespace Chilano.Iso2God
{
    using Chilano.Common;
    using Chilano.Iso2God.Ftp;
    using Chilano.Iso2God.Properties;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.IO;
    using System.Management;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;

    public class Main : Form
    {
        private ToolStripMenuItem allToolStripMenuItem;
        private ContextMenuStrip cmQueue;
        private ColumnHeader columnHeader1;
        private ColumnHeader columnHeader2;
        private ColumnHeader columnHeader3;
        private ColumnHeader columnHeader4;
        private ColumnHeader columnHeader5;
        private ColumnHeader columnHeader6;
        private ColumnHeader columnHeader7;
        private ToolStripMenuItem completedToolStripMenuItem;
        private IContainer components;
        private ToolStripMenuItem editToolStripMenuItem;
        private Timer freeDiskCheck;
        private FtpUploader ftp = new FtpUploader();
        private Timer ftpCheck;
        private Chilano.Iso2God.Iso2God i2g = new Chilano.Iso2God.Iso2God();
        private Timer jobCheck;
        private CListView listView1;
        public string pathTemp = "";
        public string pathXT = "";
        private ToolStripMenuItem removeToolStripMenuItem;
        private ToolStripMenuItem restartFTPUploadToolStripMenuItem;
        private ToolStripMenuItem selectedToolStripMenuItem;
        private StatusStrip statusStrip1;
        private ToolStrip toolStrip1;
        private ToolStripButton toolStripButton1;
        private ToolStripButton toolStripButton2;
        private ToolStripButton toolStripButton3;
        private ToolStripButton toolStripButton4;
        private ToolStripDropDownButton toolStripDropDownButton1;
        private ToolStripLabel toolStripLabel2;
        private ToolStripSeparator toolStripSeparator1;
        private ToolStripSeparator toolStripSeparator2;
        private ToolStripStatusLabel tsStatus;

        public Main()
        {
            this.InitializeComponent();
            base.Load += new EventHandler(this.Main_Load);
            base.FormClosing += new FormClosingEventHandler(this.Main_FormClosing);
            this.jobCheck.Tick += new EventHandler(this.jobCheck_Tick);
            this.ftpCheck.Tick += new EventHandler(this.ftpCheck_Tick);
            this.freeDiskCheck.Tick += new EventHandler(this.freeDiskCheck_Tick);
            this.i2g.Progress += new Iso2GodProgressEventHandler(this.i2g_Progress);
            this.i2g.Completed += new Iso2GodCompletedEventHandler(this.i2g_Completed);
            this.ftp.RunWorkerCompleted += new RunWorkerCompletedEventHandler(this.ftp_RunWorkerCompleted);
            this.ftp.ProgressChanged += new ProgressChangedEventHandler(this.ftp_ProgressChanged);
        }

        public void AddISOEntry(IsoEntry Entry)
        {
            ListViewItem item = new ListViewItem {
                Text = Entry.TitleName
            };
            item.SubItems.Add(Entry.ID.TitleID);
            item.SubItems.Add(Entry.ID.DiscNumber.ToString());
            double num = Math.Round((double) (((double) Entry.Size) / 1073741824.0), 2);
            item.SubItems.Add(num.ToString() + " GB");
            item.SubItems.Add(Entry.Padding.Type.ToString());
            item.SubItems.Add("");
            item.SubItems.Add(Entry.Path);
            item.Tag = Entry;
            this.listView1.Items.Add(item);
            this.listView1.AddEmbeddedControl(new ProgressBar(), 5, item.Index);
            long freeSpace = 0L;
            this.UpdateSpace(out freeSpace);
            if (freeSpace < Entry.Size)
            {
                this.tsStatus.Text = this.tsStatus.Text + ". You do not have enough free disk space to convert this ISO.";
            }
        }

        private void allToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (ListViewItem item in this.listView1.Items)
            {
                if (((IsoEntry) item.Tag).Status != IsoEntryStatus.InProgress)
                {
                    item.Selected = true;
                }
            }
            this.listView1.Remove(CListView.RemoveType.Selected);
        }

        private void completedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (ListViewItem item in this.listView1.SelectedItems)
            {
                if (((IsoEntry) item.Tag).Status == IsoEntryStatus.Idle)
                {
                    item.Selected = false;
                }
            }
            foreach (ListViewItem item2 in this.listView1.Items)
            {
                if (((IsoEntry) item2.Tag).Status == IsoEntryStatus.Completed)
                {
                    item2.Selected = true;
                }
            }
            this.listView1.Remove(CListView.RemoveType.Selected);
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && (this.components != null))
            {
                this.components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void editToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (this.listView1.SelectedItems.Count > 0)
            {
                this.listView1_DoubleClick(this, new EventArgs());
            }
        }

        [DllImport("user32.dll")]
        private static extern bool FlashWindow(IntPtr hwnd, bool bInvert);
        private void freeDiskCheck_Tick(object sender, EventArgs e)
        {
            this.UpdateSpace();
        }

        private void ftp_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            foreach (ListViewItem item in this.listView1.Items)
            {
                IsoEntry tag = (IsoEntry) item.Tag;
                if (tag.Status == IsoEntryStatus.Uploading)
                {
                    ProgressBar embeddedControl = (ProgressBar) this.listView1.GetEmbeddedControl(5, item.Index);
                    embeddedControl.Value = (e.ProgressPercentage > 100) ? 100 : e.ProgressPercentage;
                    item.ForeColor = Color.Blue;
                    item.SubItems[6].Text = e.UserState.ToString();
                    item.Tag = tag;
                    break;
                }
            }
        }

        private void ftp_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            foreach (ListViewItem item in this.listView1.Items)
            {
                IsoEntry tag = (IsoEntry) item.Tag;
                if (tag.Status == IsoEntryStatus.Uploading)
                {
                    tag.Status = IsoEntryStatus.Completed;
                    ProgressBar embeddedControl = (ProgressBar) this.listView1.GetEmbeddedControl(5, item.Index);
                    embeddedControl.Style = ProgressBarStyle.Continuous;
                    embeddedControl.Value = 100;
                    FlashWindow(base.Handle, false);
                    if (this.ftp.Errors.Count == 0)
                    {
                        item.ForeColor = Color.Green;
                        item.SubItems[6].Text = "Uploaded";
                    }
                    else
                    {
                        item.ForeColor = Color.Red;
                        foreach (Exception exception in this.ftp.Errors)
                        {
                            MessageBox.Show("Error while attempting to upload GOD container for '" + tag.TitleName + "':\n\n" + exception.Message);
                        }
                        item.SubItems[6].Text = "Failed to upload.";
                    }
                    item.Tag = tag;
                    this.ftpCheck.Enabled = true;
                    break;
                }
            }
        }

        private void ftpCheck_Tick(object sender, EventArgs e)
        {
            if (!this.ftp.IsBusy)
            {
                foreach (ListViewItem item in this.listView1.Items)
                {
                    IsoEntry tag = (IsoEntry) item.Tag;
                    if (tag.Status == IsoEntryStatus.UploadQueue)
                    {
                        tag.Status = IsoEntryStatus.Uploading;
                        item.Tag = tag;
                        this.ftp = new FtpUploader();
                        this.ftp.RunWorkerCompleted += new RunWorkerCompletedEventHandler(this.ftp_RunWorkerCompleted);
                        this.ftp.ProgressChanged += new ProgressChangedEventHandler(this.ftp_ProgressChanged);
                        string ip = Chilano.Iso2God.Properties.Settings.Default["FtpIP"].ToString();
                        string user = Chilano.Iso2God.Properties.Settings.Default["FtpUser"].ToString();
                        string pass = Chilano.Iso2God.Properties.Settings.Default["FtpPass"].ToString();
                        string port = Chilano.Iso2God.Properties.Settings.Default["FtpPort"].ToString();
                        string containerID = tag.ID.ContainerID;
                        this.ftp.RunWorkerAsync(new FtpUploaderArgs(ip, user, pass, port, tag.ID.TitleID, tag.ID.ContainerID, tag.Destination, tag.Platform));
                        this.ftpCheck.Enabled = false;
                        return;
                    }
                }
                this.ftpCheck.Enabled = false;
            }
        }

        public long GetFreeSpace(string DriveLetter)
        {
            try
            {
                ManagementObject obj2 = new ManagementObject("win32_logicaldisk.deviceid=\"" + DriveLetter.ToLower() + ":\"");
                obj2.Get();
                return (long) ((ulong) obj2["FreeSpace"]);
            }
            catch
            {
                return -1L;
            }
        }

        public string getVersion(bool build, bool revision)
        {
            Version version = Assembly.GetExecutingAssembly().GetName().Version;
            string str = "";
            object obj2 = str;
            str = string.Concat(new object[] { obj2, version.Major, ".", version.Minor });
            if (build)
            {
                str = str + "." + version.Build.ToString();
                if (revision)
                {
                    str = str + "." + version.Revision.ToString();
                }
            }
            return str;
        }

        private void i2g_Completed(object sender, Iso2GodCompletedArgs e)
        {
            foreach (ListViewItem item in this.listView1.Items)
            {
                IsoEntry tag = (IsoEntry) item.Tag;
                if (tag.Status == IsoEntryStatus.InProgress)
                {
                    ProgressBar embeddedControl = (ProgressBar) this.listView1.GetEmbeddedControl(5, item.Index);
                    if ((bool) Chilano.Iso2God.Properties.Settings.Default["FtpUpload"])
                    {
                        tag.Status = IsoEntryStatus.UploadQueue;
                        tag.ID.ContainerID = e.ContainerId;
                        embeddedControl.Value = 0;
                        item.SubItems[6].Text = "Queued for upload.";
                        this.ftpCheck.Enabled = true;
                    }
                    else
                    {
                        tag.Status = IsoEntryStatus.Completed;
                        embeddedControl.Value = 100;
                        item.SubItems[6].Text = e.Message + ((e.Error != null) ? (". Error: " + e.Error.Message) : "");
                        FlashWindow(base.Handle, false);
                    }
                    this.jobCheck.Enabled = true;
                    item.Tag = tag;
                    item.ForeColor = Color.Green;
                    break;
                }
            }
        }

        private void i2g_Progress(object sender, Iso2GodProgressArgs e)
        {
            foreach (ListViewItem item in this.listView1.Items)
            {
                IsoEntry tag = (IsoEntry) item.Tag;
                if (tag.Status == IsoEntryStatus.InProgress)
                {
                    ((ProgressBar) this.listView1.GetEmbeddedControl(5, item.Index)).Value = (e.Percentage > 100) ? 100 : e.Percentage;
                    item.SubItems[6].Text = e.Message;
                    item.Tag = tag;
                    break;
                }
            }
        }

        private void InitializeComponent()
        {
            this.components = new Container();
            ComponentResourceManager manager = new ComponentResourceManager(typeof(Main));
            this.toolStrip1 = new ToolStrip();
            this.toolStripLabel2 = new ToolStripLabel();
            this.toolStripDropDownButton1 = new ToolStripDropDownButton();
            this.allToolStripMenuItem = new ToolStripMenuItem();
            this.selectedToolStripMenuItem = new ToolStripMenuItem();
            this.completedToolStripMenuItem = new ToolStripMenuItem();
            this.toolStripButton2 = new ToolStripButton();
            this.toolStripSeparator1 = new ToolStripSeparator();
            this.toolStripButton3 = new ToolStripButton();
            this.toolStripButton4 = new ToolStripButton();
            this.listView1 = new CListView();
            this.columnHeader3 = new ColumnHeader();
            this.columnHeader5 = new ColumnHeader();
            this.columnHeader1 = new ColumnHeader();
            this.columnHeader2 = new ColumnHeader();
            this.columnHeader7 = new ColumnHeader();
            this.columnHeader4 = new ColumnHeader();
            this.columnHeader6 = new ColumnHeader();
            this.cmQueue = new ContextMenuStrip(this.components);
            this.editToolStripMenuItem = new ToolStripMenuItem();
            this.removeToolStripMenuItem = new ToolStripMenuItem();
            this.toolStripSeparator2 = new ToolStripSeparator();
            this.restartFTPUploadToolStripMenuItem = new ToolStripMenuItem();
            this.statusStrip1 = new StatusStrip();
            this.tsStatus = new ToolStripStatusLabel();
            this.jobCheck = new Timer(this.components);
            this.ftpCheck = new Timer(this.components);
            this.freeDiskCheck = new Timer(this.components);
            this.toolStripButton1 = new ToolStripButton();
            this.toolStrip1.SuspendLayout();
            this.cmQueue.SuspendLayout();
            this.statusStrip1.SuspendLayout();
            base.SuspendLayout();
            this.toolStrip1.AutoSize = false;
            this.toolStrip1.BackgroundImage = Resources.ToolbarBg;
            this.toolStrip1.CanOverflow = false;
            this.toolStrip1.GripStyle = ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new ToolStripItem[] { this.toolStripLabel2, this.toolStripButton1, this.toolStripDropDownButton1, this.toolStripButton2, this.toolStripSeparator1, this.toolStripButton3, this.toolStripButton4 });
            this.toolStrip1.Location = new Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Padding = new Padding(0);
            this.toolStrip1.Size = new Size(0x2ba, 70);
            this.toolStrip1.TabIndex = 0;
            this.toolStrip1.Text = "toolStrip1";
            this.toolStripLabel2.BackgroundImageLayout = ImageLayout.Center;
            this.toolStripLabel2.Image = Resources.LogoToolbar;
            this.toolStripLabel2.ImageScaling = ToolStripItemImageScaling.None;
            this.toolStripLabel2.Margin = new Padding(10, 0, 0, 0);
            this.toolStripLabel2.Name = "toolStripLabel2";
            this.toolStripLabel2.Size = new Size(0xc2, 70);
            this.toolStripDropDownButton1.DropDownItems.AddRange(new ToolStripItem[] { this.allToolStripMenuItem, this.selectedToolStripMenuItem, this.completedToolStripMenuItem });
            this.toolStripDropDownButton1.Image = Resources.No_entry;
            this.toolStripDropDownButton1.ImageScaling = ToolStripItemImageScaling.None;
            this.toolStripDropDownButton1.ImageTransparentColor = Color.Magenta;
            this.toolStripDropDownButton1.Margin = new Padding(5, 1, 0, 2);
            this.toolStripDropDownButton1.Name = "toolStripDropDownButton1";
            this.toolStripDropDownButton1.Size = new Size(0x57, 0x43);
            this.toolStripDropDownButton1.Text = "Remove";
            this.allToolStripMenuItem.Name = "allToolStripMenuItem";
            this.allToolStripMenuItem.Size = new Size(0x98, 0x16);
            this.allToolStripMenuItem.Text = "All";
            this.allToolStripMenuItem.Click += new EventHandler(this.allToolStripMenuItem_Click);
            this.selectedToolStripMenuItem.Name = "selectedToolStripMenuItem";
            this.selectedToolStripMenuItem.Size = new Size(0x98, 0x16);
            this.selectedToolStripMenuItem.Text = "Selected";
            this.selectedToolStripMenuItem.Click += new EventHandler(this.selectedToolStripMenuItem_Click);
            this.completedToolStripMenuItem.Name = "completedToolStripMenuItem";
            this.completedToolStripMenuItem.Size = new Size(0x98, 0x16);
            this.completedToolStripMenuItem.Text = "Completed";
            this.completedToolStripMenuItem.Click += new EventHandler(this.completedToolStripMenuItem_Click);
            this.toolStripButton2.Image = Resources.Go;
            this.toolStripButton2.ImageScaling = ToolStripItemImageScaling.None;
            this.toolStripButton2.ImageTransparentColor = Color.Magenta;
            this.toolStripButton2.Margin = new Padding(5, 1, 0, 2);
            this.toolStripButton2.Name = "toolStripButton2";
            this.toolStripButton2.Size = new Size(0x4d, 0x43);
            this.toolStripButton2.Text = "Convert";
            this.toolStripButton2.Click += new EventHandler(this.toolStripButton2_Click);
            this.toolStripSeparator1.Margin = new Padding(10, 0, 10, 0);
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new Size(6, 70);
            this.toolStripButton3.Image = Resources.Application;
            this.toolStripButton3.ImageScaling = ToolStripItemImageScaling.None;
            this.toolStripButton3.ImageTransparentColor = Color.Magenta;
            this.toolStripButton3.Margin = new Padding(5, 1, 0, 2);
            this.toolStripButton3.Name = "toolStripButton3";
            this.toolStripButton3.Size = new Size(0x4d, 0x43);
            this.toolStripButton3.Text = "Settings";
            this.toolStripButton3.Click += new EventHandler(this.toolStripButton3_Click);
            this.toolStripButton4.Image = Resources.Info;
            this.toolStripButton4.ImageScaling = ToolStripItemImageScaling.None;
            this.toolStripButton4.ImageTransparentColor = Color.Magenta;
            this.toolStripButton4.Margin = new Padding(5, 1, 0, 2);
            this.toolStripButton4.Name = "toolStripButton4";
            this.toolStripButton4.Size = new Size(0x44, 0x43);
            this.toolStripButton4.Text = "About";
            this.toolStripButton4.Click += new EventHandler(this.toolStripButton4_Click);
            this.listView1.BorderStyle = BorderStyle.None;
            this.listView1.Columns.AddRange(new ColumnHeader[] { this.columnHeader3, this.columnHeader5, this.columnHeader1, this.columnHeader2, this.columnHeader7, this.columnHeader4, this.columnHeader6 });
            this.listView1.ContextMenuStrip = this.cmQueue;
            this.listView1.Dock = DockStyle.Top;
            this.listView1.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.listView1.FullRowSelect = true;
            this.listView1.GridLines = true;
            this.listView1.Location = new Point(0, 70);
            this.listView1.Name = "listView1";
            this.listView1.Size = new Size(0x2ba, 180);
            this.listView1.TabIndex = 1;
            this.listView1.UseCompatibleStateImageBehavior = false;
            this.listView1.View = View.Details;
            this.columnHeader3.Text = "Name";
            this.columnHeader3.Width = 0xaf;
            this.columnHeader5.Text = "Title ID";
            this.columnHeader5.Width = 0x44;
            this.columnHeader1.Text = "Disc";
            this.columnHeader1.Width = 0x24;
            this.columnHeader2.Text = "Size";
            this.columnHeader2.Width = 0x34;
            this.columnHeader7.Text = "Padding";
            this.columnHeader7.Width = 0x3a;
            this.columnHeader4.Text = "Progress";
            this.columnHeader4.Width = 0x6b;
            this.columnHeader6.Text = "Status Message";
            this.columnHeader6.Width = 0xb8;
            this.cmQueue.Items.AddRange(new ToolStripItem[] { this.editToolStripMenuItem, this.removeToolStripMenuItem, this.toolStripSeparator2, this.restartFTPUploadToolStripMenuItem });
            this.cmQueue.Name = "cmQueue";
            this.cmQueue.Size = new Size(0xaf, 0x4c);
            this.editToolStripMenuItem.Name = "editToolStripMenuItem";
            this.editToolStripMenuItem.Size = new Size(0xae, 0x16);
            this.editToolStripMenuItem.Text = "Edit";
            this.editToolStripMenuItem.Click += new EventHandler(this.editToolStripMenuItem_Click);
            this.removeToolStripMenuItem.Name = "removeToolStripMenuItem";
            this.removeToolStripMenuItem.Size = new Size(0xae, 0x16);
            this.removeToolStripMenuItem.Text = "Remove";
            this.removeToolStripMenuItem.Click += new EventHandler(this.removeToolStripMenuItem_Click);
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new Size(0xab, 6);
            this.restartFTPUploadToolStripMenuItem.Name = "restartFTPUploadToolStripMenuItem";
            this.restartFTPUploadToolStripMenuItem.Size = new Size(0xae, 0x16);
            this.restartFTPUploadToolStripMenuItem.Text = "Restart FTP Upload";
            this.restartFTPUploadToolStripMenuItem.Click += new EventHandler(this.restartFTPUploadToolStripMenuItem_Click);
            this.statusStrip1.Items.AddRange(new ToolStripItem[] { this.tsStatus });
            this.statusStrip1.Location = new Point(0, 0xfc);
            this.statusStrip1.Name = "statusStrip1";
            this.statusStrip1.Size = new Size(0x2ba, 0x16);
            this.statusStrip1.SizingGrip = false;
            this.statusStrip1.TabIndex = 2;
            this.statusStrip1.Text = "statusStrip1";
            this.tsStatus.Name = "tsStatus";
            this.tsStatus.Size = new Size(0x1a, 0x11);
            this.tsStatus.Text = "Idle";
            this.freeDiskCheck.Enabled = true;
            this.freeDiskCheck.Interval = 0x7530;
            this.toolStripButton1.Image = Resources.Create;
            this.toolStripButton1.ImageScaling = ToolStripItemImageScaling.None;
            this.toolStripButton1.ImageTransparentColor = Color.Magenta;
            this.toolStripButton1.Margin = new Padding(0x19, 1, 0, 2);
            this.toolStripButton1.Name = "toolStripButton1";
            this.toolStripButton1.Size = new Size(0x4e, 0x43);
            this.toolStripButton1.Text = "Add ISO";
            this.toolStripButton1.Click += new EventHandler(this.toolStripButton1_Click);
            base.AutoScaleDimensions = new SizeF(6f, 13f);
            base.AutoScaleMode = AutoScaleMode.Font;
            base.ClientSize = new Size(0x2ba, 0x112);
            base.Controls.Add(this.statusStrip1);
            base.Controls.Add(this.listView1);
            base.Controls.Add(this.toolStrip1);
            base.FormBorderStyle = FormBorderStyle.FixedSingle;
            base.Icon = (Icon) manager.GetObject("$this.Icon");
            base.MaximizeBox = false;
            base.Name = "Main";
            base.StartPosition = FormStartPosition.CenterScreen;
            this.Text = "Iso2God";
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.cmQueue.ResumeLayout(false);
            this.statusStrip1.ResumeLayout(false);
            this.statusStrip1.PerformLayout();
            base.ResumeLayout(false);
            base.PerformLayout();
        }

        public bool IsRunningOnMono()
        {
            return (System.Type.GetType("Mono.Runtime") != null);
        }

        private void jobCheck_Tick(object sender, EventArgs e)
        {
            if (!this.i2g.IsBusy)
            {
                foreach (ListViewItem item in this.listView1.Items)
                {
                    IsoEntry tag = (IsoEntry) item.Tag;
                    if (tag.Status == IsoEntryStatus.Idle)
                    {
                        tag.Status = IsoEntryStatus.InProgress;
                        item.Tag = tag;
                        this.i2g = new Chilano.Iso2God.Iso2God();
                        this.i2g.Completed += new Iso2GodCompletedEventHandler(this.i2g_Completed);
                        this.i2g.Progress += new Iso2GodProgressEventHandler(this.i2g_Progress);
                        this.i2g.RunWorkerAsync(tag);
                        this.jobCheck.Enabled = false;
                        return;
                    }
                }
                this.jobCheck.Enabled = false;
            }
        }

        private void listView1_DoubleClick(object sender, EventArgs e)
        {
            if (this.listView1.SelectedItems.Count <= 1)
            {
                ListViewItem item = this.listView1.SelectedItems[0];
                IsoEntry tag = (IsoEntry) item.Tag;
                if (tag.Status == IsoEntryStatus.Idle)
                {
                    using (AddISO diso = new AddISO(tag.Platform))
                    {
                        diso.Edit(item.Index, tag);
                        diso.ShowDialog(this);
                        return;
                    }
                }
                MessageBox.Show("Conversions that are currently in progress or have completed cannot be edited.");
            }
        }

        private void Main_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (this.i2g.IsBusy && (MessageBox.Show("An ISO is currently being converted.\n\nAre you sure you wish to exit?", "ISO Conversion in Progress", MessageBoxButtons.YesNo) == DialogResult.No))
            {
                e.Cancel = true;
            }
            else if (this.ftp.IsBusy && (MessageBox.Show("A GOD container is currently being uploaded.\n\nAre you sure you wish to exit?", "FTP Upload in Progress", MessageBoxButtons.YesNo) == DialogResult.No))
            {
                e.Cancel = true;
            }
            else if (Directory.Exists(this.pathTemp))
            {
                foreach (string str in Directory.GetFiles(this.pathTemp))
                {
                    File.Delete(str);
                }
            }
        }

        private void Main_Load(object sender, EventArgs e)
        {
            this.listView1.DoubleClick += new EventHandler(this.listView1_DoubleClick);
            this.Text = this.Text + " " + this.getVersion(true, false);
            this.pathTemp = Path.GetTempPath() + "i2g" + Path.DirectorySeparatorChar;
            Directory.CreateDirectory(this.pathTemp);
            this.pathXT = Application.StartupPath + Path.DirectorySeparatorChar + "xextool.exe";
            if (!File.Exists(this.pathXT))
            {
                this.tsStatus.Text = "Could not locate XexTool! Please ensure it is in the same directory as Iso2God or thumbnail extraction will not work.";
            }
            else
            {
                this.UpdateSpace();
            }
        }

        private void removeToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (ListViewItem item in this.listView1.SelectedItems)
            {
                this.listView1.Items.Remove(item);
            }
        }

        private void restartFTPUploadToolStripMenuItem_Click(object sender, EventArgs e)
        {
            if (this.listView1.SelectedItems.Count == 1)
            {
                ListViewItem item = this.listView1.SelectedItems[0];
                IsoEntry tag = (IsoEntry) item.Tag;
                if ((tag.Status == IsoEntryStatus.Uploading) || (tag.Status == IsoEntryStatus.Completed))
                {
                    tag.Status = IsoEntryStatus.UploadQueue;
                    item.Tag = tag;
                    item.ForeColor = Color.Blue;
                    item.SubItems[6].Text = "Queued for upload.";
                    ((ProgressBar) this.listView1.GetEmbeddedControl(5, item.Index)).Value = 0;
                    this.ftpCheck.Enabled = true;
                }
            }
        }

        private void selectedToolStripMenuItem_Click(object sender, EventArgs e)
        {
            foreach (ListViewItem item in this.listView1.Items)
            {
                if (((IsoEntry) item.Tag).Status == IsoEntryStatus.InProgress)
                {
                    item.Selected = false;
                }
            }
            this.listView1.Remove(CListView.RemoveType.Selected);
        }

        private void toolStripButton1_Click(object sender, EventArgs e)
        {
            using (AddISO diso = new AddISO(IsoEntryPlatform.Xbox360))
            {
                diso.ShowDialog(this);
            }
        }

        private void toolStripButton2_Click(object sender, EventArgs e)
        {
            this.jobCheck.Enabled = true;
        }

        private void toolStripButton3_Click(object sender, EventArgs e)
        {
            using (Chilano.Iso2God.Settings settings = new Chilano.Iso2God.Settings())
            {
                settings.ShowDialog(this);
            }
        }

        private void toolStripButton4_Click(object sender, EventArgs e)
        {
            using (About about = new About())
            {
                about.ShowDialog(this);
            }
        }

        public void UpdateISOEntry(int Index, IsoEntry Entry)
        {
            ListViewItem item = this.listView1.Items[Index];
            item.Tag = Entry;
            item.Text = Entry.TitleName;
            item.SubItems[1].Text = Entry.ID.TitleID;
            item.SubItems[2].Text = Entry.ID.DiscNumber.ToString();
            item.SubItems[3].Text = Math.Round((double) (((double) Entry.Size) / 1073741824.0), 2).ToString() + " GB";
            item.SubItems[4].Text = Entry.Padding.Type.ToString();
            item.SubItems[6].Text = Entry.Path;
        }

        public void UpdateSpace()
        {
            long num;
            this.UpdateSpace(out num);
        }

        public void UpdateSpace(out long FreeSpace)
        {
            string str = Chilano.Iso2God.Properties.Settings.Default["OutputPath"].ToString();
            if (!this.IsRunningOnMono())
            {
                FreeSpace = this.GetFreeSpace((str.Length > 0) ? str[0].ToString() : "C");
                if (FreeSpace > -1L)
                {
                    this.tsStatus.Text = "Free Disk Space: " + Math.Round((double) (((float) FreeSpace) / 1.073742E+09f), 2).ToString() + " GB";
                }
                else
                {
                    this.tsStatus.Text = "Free Disk Space: Unable to find this value.";
                }
            }
            else
            {
                this.tsStatus.Text = "Free Disk Space: [Not supported for Mono. Sorry!]";
                FreeSpace = 0L;
            }
        }
    }
}

