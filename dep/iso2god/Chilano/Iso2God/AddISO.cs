namespace Chilano.Iso2God
{
    using Chilano.Iso2God.Properties;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.Drawing.Imaging;
    using System.IO;
    using System.Windows.Forms;

    public class AddISO : Form
    {
        private Button btnAddIso;
        private Button btnCancel;
        private Button btnDestBrowse;
        private Button btnISOBrowse;
        private Button btnRebuiltBrowse;
        private CheckBox cbSaveRebuilt;
        private ComboBox cmbPaddingMode;
        private IContainer components;
        private bool edit;
        private IsoEntry entry;
        private int entryIndex;
        private GroupBox groupBox1;
        private GroupBox groupBox2;
        private GroupBox groupBox3;
        private IsoDetails isoDetails = new IsoDetails();
        private Label label1;
        private Label label12;
        private Label label14;
        private Label label2;
        private Label label3;
        private Label label4;
        private Label label5;
        private Label label6;
        private Label label7;
        private Label label8;
        private Label label9;
        private PictureBox pbPadding;
        private PictureBox pbThumb;
        private PictureBox pbTime;
        private PictureBox pbVideo;
        private IsoEntryPlatform platform = IsoEntryPlatform.Xbox360;
        private ToolTip ttISO;
        private ToolTip ttPadding;
        private ToolTip ttSettings;
        private ToolTip ttThumb;
        private TextBox txtDest;
        private TextBox txtDiscCount;
        private TextBox txtDiscNum;
        private TextBox txtExType;
        private TextBox txtISO;
        private TextBox txtMediaID;
        private TextBox txtName;
        private TextBox txtPlatform;
        private TextBox txtRebuiltIso;
        private TextBox txtTitleID;

        public AddISO(IsoEntryPlatform Platform)
        {
            this.InitializeComponent();
            base.Shown += new EventHandler(this.AddISO_Shown);
            this.cmbPaddingMode.SelectedIndex = (int) Chilano.Iso2God.Properties.Settings.Default["DefaultPadding"];
            this.platform = Platform;
            this.entry.Platform = this.platform;
            this.isoDetails.ProgressChanged += new ProgressChangedEventHandler(this.isoDetails_ProgressChanged);
            this.isoDetails.RunWorkerCompleted += new RunWorkerCompletedEventHandler(this.isoDetails_RunWorkerCompleted);
            this.txtDest.Text = Chilano.Iso2God.Properties.Settings.Default["OutputPath"].ToString();
            this.txtRebuiltIso.Text = Chilano.Iso2God.Properties.Settings.Default["RebuildPath"].ToString();
            this.cbSaveRebuilt.Checked = (bool) Chilano.Iso2God.Properties.Settings.Default["AlwaysSave"];
            this.ttISO.SetToolTip(this.pbVideo, "Select the ISO image you want to convert to a Games on Demand package.\n\nChoose a location to output the GOD package to. It will be written into a\nsub-directory named using the TitleID of the ISO's default.xex. A default\nlocation can be set in the Settings screen.");
            this.ttSettings.SetToolTip(this.pbTime, "The details are automatically extracted from default.xex in the root\ndirectory of the ISO image. They can be manually altered if required.\n\nThe Title Name MUST be entered, or else it will show up as an \nunknown game on the 360.");
            this.ttThumb.SetToolTip(this.pbThumb, "Click to set a custom thumbnail for this title.");
            this.ttPadding.SetToolTip(this.pbPadding, "Unused padding sectors can be removed from the ISO image when it is converted.\n\nThree modes are available:\n\nNone - ISO image is left untouched.\n\nPartial - The ISO image is cropped after the end of the last used sector. Very quick to do,\n              but it will only save 800-1500MB of space.\n\nFull - ISO image is processed and completely rebuilt to remove all padding. Rebuilt image is\n         can be stored temporarily or kept for future use. Takes 5-10 minutes extra.");
            this.txtISO.Focus();
        }

        private void AddISO_Shown(object sender, EventArgs e)
        {
            if (!this.edit && ((bool) Chilano.Iso2God.Properties.Settings.Default["AutoBrowse"]))
            {
                this.btnISOBrowse_Click(base.Owner, null);
            }
        }

        private void btnDestBrowse_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog {
                RootFolder = Environment.SpecialFolder.Desktop,
                ShowNewFolderButton = true,
                Description = "Choose where to save the GOD Package to:"
            };
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                this.txtDest.Text = dialog.SelectedPath;
                if (!this.txtDest.Text.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    this.txtDest.Text = this.txtDest.Text + Path.DirectorySeparatorChar;
                }
            }
        }

        private void btnISOBrowse_Click(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog {
                InitialDirectory = Environment.SpecialFolder.Desktop.ToString(),
                Title = "Choose location of your ISO.",
                Multiselect = false,
                Filter = "ISO Images (*.iso, *.000)|*.iso;*.000"
            };
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                this.txtISO.Text = dialog.FileName;
                this.clearXexFields();
                switch (this.entry.Platform)
                {
                    case IsoEntryPlatform.Xbox:
                        this.isoDetails.RunWorkerAsync(new IsoDetailsArgs(this.txtISO.Text, (base.Owner as Main).pathTemp, (base.Owner as Main).pathXT));
                        this.txtName.Text = "Reading default.xbe...";
                        break;

                    case IsoEntryPlatform.Xbox360:
                        this.isoDetails.RunWorkerAsync(new IsoDetailsArgs(this.txtISO.Text, (base.Owner as Main).pathTemp, (base.Owner as Main).pathXT));
                        this.txtName.Text = "Reading default.xex...";
                        return;

                    default:
                        return;
                }
            }
        }

        private void btnRebuiltBrowse_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog {
                RootFolder = Environment.SpecialFolder.Desktop,
                ShowNewFolderButton = true,
                Description = "Choose where to save the rebuilt ISO to:"
            };
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                this.txtRebuiltIso.Text = dialog.SelectedPath;
                if (!this.txtRebuiltIso.Text.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    this.txtRebuiltIso.Text = this.txtRebuiltIso.Text + Path.DirectorySeparatorChar;
                }
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            base.Close();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if (this.checkFields())
            {
                IsoEntryPadding padding = new IsoEntryPadding {
                    Type = (IsoEntryPaddingRemoval) ((byte) this.cmbPaddingMode.SelectedIndex),
                    TempPath = Path.GetTempPath(),
                    IsoPath = this.txtRebuiltIso.Text,
                    KeepIso = this.cbSaveRebuilt.Checked
                };
                if (!padding.TempPath.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    padding.TempPath = padding.TempPath + Path.DirectorySeparatorChar;
                }
                if (!padding.IsoPath.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    padding.IsoPath = padding.IsoPath + Path.DirectorySeparatorChar;
                }
                IsoEntryID iD = new IsoEntryID(this.txtTitleID.Text, this.txtMediaID.Text, byte.Parse(this.txtDiscNum.Text), byte.Parse(this.txtDiscCount.Text), byte.Parse(this.txtPlatform.Text), byte.Parse(this.txtExType.Text));
                FileInfo info = new FileInfo(this.txtISO.Text);
                IsoEntry entry = new IsoEntry(this.platform, this.txtISO.Text, this.txtDest.Text, info.Length, this.txtName.Text, iD, (byte[]) this.pbThumb.Tag, padding);
                if (this.edit)
                {
                    (base.Owner as Main).UpdateISOEntry(this.entryIndex, entry);
                }
                else
                {
                    (base.Owner as Main).AddISOEntry(entry);
                }
                GC.Collect();
                base.Close();
            }
        }

        private void cbSaveRebuilt_CheckedChanged(object sender, EventArgs e)
        {
            if (!((bool) Chilano.Iso2God.Properties.Settings.Default["AlwaysSave"]) && ((this.cmbPaddingMode.SelectedIndex == 0) || (this.cmbPaddingMode.SelectedIndex == 1)))
            {
                this.cbSaveRebuilt.Checked = false;
            }
        }

        private bool checkFields()
        {
            if ((this.txtDest.Text.Length == 0) || (this.txtISO.Text.Length == 0))
            {
                MessageBox.Show("Please select an ISO image to convert and a destination folder\nto store the GOD container in.");
                return false;
            }
            if (((this.txtTitleID.Text.Length != 8) || (this.txtMediaID.Text.Length != 8)) || ((this.txtExType.Text.Length == 0) || (this.txtPlatform.Text.Length == 0)))
            {
                MessageBox.Show("If you are overriding the Title Info fields, please ensure the Title and Media IDs are\n8 character Hex strings. All other fields should be no more than 1 character.\n\nIf these details were all blank after choosing an ISO image, an error has occured\nso the conversion is unlikely to work.");
                return false;
            }
            if (this.txtName.Text.Length == 0)
            {
                MessageBox.Show("The name of the game is currently not automatically detected.\n\nPlease enter this manually in the Name field above.");
                return false;
            }
            if (this.cmbPaddingMode.SelectedIndex == 2)
            {
                if ((!this.cbSaveRebuilt.Checked && ((bool) Chilano.Iso2God.Properties.Settings.Default["RebuiltCheck"])) && (MessageBox.Show("Are you sure you want to discard the temporary ISO after it has been rebuilt?", "Discard Temporary ISO Image", MessageBoxButtons.YesNo) == DialogResult.No))
                {
                    return false;
                }
                if (this.txtRebuiltIso.Text.Length == 0)
                {
                    MessageBox.Show("You must enter a location for the temporary ISO image to be stored at.");
                    return false;
                }
            }
            return true;
        }

        private void clearXexFields()
        {
            this.txtTitleID.Text = "";
            this.txtMediaID.Text = "";
            this.txtDiscNum.Text = "";
            this.txtDiscCount.Text = "";
            this.txtExType.Text = "";
            this.txtPlatform.Text = "";
            this.pbThumb.Image = null;
            this.pbThumb.Tag = null;
            this.txtName.Text = "";
        }

        private void cmbPaddingMode_SelectedIndexChanged(object sender, EventArgs e)
        {
            if ((this.cmbPaddingMode.SelectedIndex == 0) || (this.cmbPaddingMode.SelectedIndex == 1))
            {
                if (!((bool) Chilano.Iso2God.Properties.Settings.Default["AlwaysSave"]))
                {
                    this.cbSaveRebuilt.Checked = false;
                }
                this.txtRebuiltIso.Enabled = false;
                this.btnRebuiltBrowse.Enabled = false;
            }
            else
            {
                this.txtRebuiltIso.Enabled = true;
                this.btnRebuiltBrowse.Enabled = true;
            }
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && (this.components != null))
            {
                this.components.Dispose();
            }
            base.Dispose(disposing);
        }

        public void Edit(int Index, IsoEntry Entry)
        {
            this.edit = true;
            this.entry = Entry;
            this.entryIndex = Index;
            this.txtName.Text = this.entry.TitleName;
            this.txtTitleID.Text = this.entry.ID.TitleID;
            this.txtMediaID.Text = this.entry.ID.MediaID;
            this.txtPlatform.Text = this.entry.ID.Platform.ToString();
            this.txtExType.Text = this.entry.ID.ExType.ToString();
            this.txtDiscCount.Text = this.entry.ID.DiscCount.ToString();
            this.txtDiscNum.Text = this.entry.ID.DiscNumber.ToString();
            this.txtDest.Text = this.entry.Destination;
            this.txtISO.Text = this.entry.Path;
            this.txtRebuiltIso.Text = (this.entry.Padding.Type == IsoEntryPaddingRemoval.Full) ? this.entry.Padding.IsoPath : Chilano.Iso2God.Properties.Settings.Default["RebuildPath"].ToString();
            this.cbSaveRebuilt.Checked = this.entry.Padding.KeepIso;
            this.cmbPaddingMode.SelectedIndex = (int) this.entry.Padding.Type;
            this.pbThumb.Image = (this.entry.Thumb == null) ? null : Image.FromStream(new MemoryStream(this.entry.Thumb));
            this.pbThumb.Tag = this.entry.Thumb;
            this.btnAddIso.Text = "Save ISO";
        }

        private void InitializeComponent()
        {
            this.components = new Container();
            this.groupBox2 = new GroupBox();
            this.btnISOBrowse = new Button();
            this.label1 = new Label();
            this.txtISO = new TextBox();
            this.pbVideo = new PictureBox();
            this.btnDestBrowse = new Button();
            this.label3 = new Label();
            this.txtDest = new TextBox();
            this.ttISO = new ToolTip(this.components);
            this.groupBox3 = new GroupBox();
            this.pbThumb = new PictureBox();
            this.label8 = new Label();
            this.label9 = new Label();
            this.txtExType = new TextBox();
            this.txtPlatform = new TextBox();
            this.label5 = new Label();
            this.txtName = new TextBox();
            this.label6 = new Label();
            this.txtDiscCount = new TextBox();
            this.txtDiscNum = new TextBox();
            this.label4 = new Label();
            this.label2 = new Label();
            this.txtMediaID = new TextBox();
            this.txtTitleID = new TextBox();
            this.pbTime = new PictureBox();
            this.label7 = new Label();
            this.btnAddIso = new Button();
            this.btnCancel = new Button();
            this.ttSettings = new ToolTip(this.components);
            this.ttThumb = new ToolTip(this.components);
            this.groupBox1 = new GroupBox();
            this.cbSaveRebuilt = new CheckBox();
            this.btnRebuiltBrowse = new Button();
            this.cmbPaddingMode = new ComboBox();
            this.label12 = new Label();
            this.label14 = new Label();
            this.txtRebuiltIso = new TextBox();
            this.pbPadding = new PictureBox();
            this.ttPadding = new ToolTip(this.components);
            this.groupBox2.SuspendLayout();
            ((ISupportInitialize) this.pbVideo).BeginInit();
            this.groupBox3.SuspendLayout();
            ((ISupportInitialize) this.pbThumb).BeginInit();
            ((ISupportInitialize) this.pbTime).BeginInit();
            this.groupBox1.SuspendLayout();
            ((ISupportInitialize) this.pbPadding).BeginInit();
            base.SuspendLayout();
            this.groupBox2.BackColor = SystemColors.Control;
            this.groupBox2.Controls.Add(this.btnISOBrowse);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Controls.Add(this.txtISO);
            this.groupBox2.Controls.Add(this.pbVideo);
            this.groupBox2.Controls.Add(this.btnDestBrowse);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Controls.Add(this.txtDest);
            this.groupBox2.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.groupBox2.ForeColor = SystemColors.ControlText;
            this.groupBox2.Location = new Point(13, 12);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new Size(0x183, 0x74);
            this.groupBox2.TabIndex = 12;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "ISO Details";
            this.btnISOBrowse.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.btnISOBrowse.Location = new Point(0x12f, 0x25);
            this.btnISOBrowse.Name = "btnISOBrowse";
            this.btnISOBrowse.Size = new Size(0x4b, 0x17);
            this.btnISOBrowse.TabIndex = 1;
            this.btnISOBrowse.Text = "&Browse";
            this.btnISOBrowse.UseVisualStyleBackColor = true;
            this.btnISOBrowse.Click += new EventHandler(this.btnISOBrowse_Click);
            this.label1.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label1.Location = new Point(6, 0x12);
            this.label1.Name = "label1";
            this.label1.Size = new Size(0x174, 0x10);
            this.label1.TabIndex = 0x1c;
            this.label1.Text = "Image Location:";
            this.txtISO.Enabled = false;
            this.txtISO.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtISO.Location = new Point(9, 0x26);
            this.txtISO.Name = "txtISO";
            this.txtISO.Size = new Size(0x120, 0x16);
            this.txtISO.TabIndex = 0;
            this.pbVideo.BackgroundImage = Resources.Hint;
            this.pbVideo.BackgroundImageLayout = ImageLayout.Zoom;
            this.pbVideo.Location = new Point(70, 0);
            this.pbVideo.Name = "pbVideo";
            this.pbVideo.Size = new Size(15, 15);
            this.pbVideo.TabIndex = 0x19;
            this.pbVideo.TabStop = false;
            this.btnDestBrowse.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.btnDestBrowse.Location = new Point(0x12f, 0x51);
            this.btnDestBrowse.Name = "btnDestBrowse";
            this.btnDestBrowse.Size = new Size(0x4b, 0x17);
            this.btnDestBrowse.TabIndex = 3;
            this.btnDestBrowse.Text = "&Browse";
            this.btnDestBrowse.UseVisualStyleBackColor = true;
            this.btnDestBrowse.Click += new EventHandler(this.btnDestBrowse_Click);
            this.label3.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label3.Location = new Point(6, 0x3f);
            this.label3.Name = "label3";
            this.label3.Size = new Size(0x174, 0x10);
            this.label3.TabIndex = 9;
            this.label3.Text = "Output Location:";
            this.txtDest.Enabled = false;
            this.txtDest.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtDest.Location = new Point(9, 0x52);
            this.txtDest.Name = "txtDest";
            this.txtDest.Size = new Size(0x120, 0x16);
            this.txtDest.TabIndex = 2;
            this.ttISO.AutoPopDelay = 0x2710;
            this.ttISO.InitialDelay = 100;
            this.ttISO.IsBalloon = true;
            this.ttISO.ReshowDelay = 100;
            this.groupBox3.BackColor = SystemColors.Control;
            this.groupBox3.Controls.Add(this.pbThumb);
            this.groupBox3.Controls.Add(this.label8);
            this.groupBox3.Controls.Add(this.label9);
            this.groupBox3.Controls.Add(this.txtExType);
            this.groupBox3.Controls.Add(this.txtPlatform);
            this.groupBox3.Controls.Add(this.label5);
            this.groupBox3.Controls.Add(this.txtName);
            this.groupBox3.Controls.Add(this.label6);
            this.groupBox3.Controls.Add(this.txtDiscCount);
            this.groupBox3.Controls.Add(this.txtDiscNum);
            this.groupBox3.Controls.Add(this.label4);
            this.groupBox3.Controls.Add(this.label2);
            this.groupBox3.Controls.Add(this.txtMediaID);
            this.groupBox3.Controls.Add(this.txtTitleID);
            this.groupBox3.Controls.Add(this.pbTime);
            this.groupBox3.Controls.Add(this.label7);
            this.groupBox3.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.groupBox3.ForeColor = SystemColors.ControlText;
            this.groupBox3.Location = new Point(13, 0x86);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new Size(0x183, 0x70);
            this.groupBox3.TabIndex = 0x11;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "Title Details";
            this.pbThumb.BackgroundImageLayout = ImageLayout.Center;
            this.pbThumb.BorderStyle = BorderStyle.FixedSingle;
            this.pbThumb.Location = new Point(0x135, 0x1c);
            this.pbThumb.Name = "pbThumb";
            this.pbThumb.Size = new Size(0x40, 0x40);
            this.pbThumb.TabIndex = 40;
            this.pbThumb.TabStop = false;
            this.pbThumb.Click += new EventHandler(this.pictureBox1_Click);
            this.label8.AutoSize = true;
            this.label8.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label8.Location = new Point(0xf8, 0x34);
            this.label8.Name = "label8";
            this.label8.Size = new Size(11, 13);
            this.label8.TabIndex = 0x27;
            this.label8.Text = "/";
            this.label9.AutoSize = true;
            this.label9.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label9.Location = new Point(0xae, 0x34);
            this.label9.Name = "label9";
            this.label9.Size = new Size(0x1f, 13);
            this.label9.TabIndex = 0x26;
            this.label9.Text = "Disc:";
            this.txtExType.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtExType.Location = new Point(0x10d, 0x4d);
            this.txtExType.Name = "txtExType";
            this.txtExType.Size = new Size(0x1c, 0x16);
            this.txtExType.TabIndex = 10;
            this.txtPlatform.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtPlatform.Location = new Point(0xd1, 0x4d);
            this.txtPlatform.Name = "txtPlatform";
            this.txtPlatform.Size = new Size(0x1c, 0x16);
            this.txtPlatform.TabIndex = 9;
            this.label5.AutoSize = true;
            this.label5.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label5.Location = new Point(6, 0x18);
            this.label5.Name = "label5";
            this.label5.Size = new Size(0x27, 13);
            this.label5.TabIndex = 0x1f;
            this.label5.Text = "Name:";
            this.txtName.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtName.Location = new Point(0x43, 0x15);
            this.txtName.Name = "txtName";
            this.txtName.Size = new Size(230, 0x16);
            this.txtName.TabIndex = 4;
            this.label6.AutoSize = true;
            this.label6.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label6.Location = new Point(0xf3, 80);
            this.label6.Name = "label6";
            this.label6.Size = new Size(0x15, 13);
            this.label6.TabIndex = 0x23;
            this.label6.Text = "Ex:";
            this.txtDiscCount.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtDiscCount.Location = new Point(0x10d, 0x31);
            this.txtDiscCount.Name = "txtDiscCount";
            this.txtDiscCount.Size = new Size(0x1c, 0x16);
            this.txtDiscCount.TabIndex = 7;
            this.txtDiscNum.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtDiscNum.Location = new Point(0xd1, 0x31);
            this.txtDiscNum.Name = "txtDiscNum";
            this.txtDiscNum.Size = new Size(0x1c, 0x16);
            this.txtDiscNum.TabIndex = 6;
            this.label4.AutoSize = true;
            this.label4.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label4.Location = new Point(6, 80);
            this.label4.Name = "label4";
            this.label4.Size = new Size(0x38, 13);
            this.label4.TabIndex = 0x1d;
            this.label4.Text = "Media ID:";
            this.label2.AutoSize = true;
            this.label2.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label2.Location = new Point(6, 0x34);
            this.label2.Name = "label2";
            this.label2.Size = new Size(0x2d, 13);
            this.label2.TabIndex = 0x1c;
            this.label2.Text = "Title ID:";
            this.txtMediaID.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtMediaID.Location = new Point(0x43, 0x4d);
            this.txtMediaID.Name = "txtMediaID";
            this.txtMediaID.Size = new Size(90, 0x16);
            this.txtMediaID.TabIndex = 8;
            this.txtTitleID.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtTitleID.Location = new Point(0x43, 0x31);
            this.txtTitleID.Name = "txtTitleID";
            this.txtTitleID.Size = new Size(90, 0x16);
            this.txtTitleID.TabIndex = 5;
            this.pbTime.BackgroundImage = Resources.Hint;
            this.pbTime.BackgroundImageLayout = ImageLayout.Zoom;
            this.pbTime.Location = new Point(0x4b, 0);
            this.pbTime.Name = "pbTime";
            this.pbTime.Size = new Size(15, 15);
            this.pbTime.TabIndex = 0x19;
            this.pbTime.TabStop = false;
            this.label7.AutoSize = true;
            this.label7.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label7.Location = new Point(0xae, 80);
            this.label7.Name = "label7";
            this.label7.Size = new Size(0x1d, 13);
            this.label7.TabIndex = 0x22;
            this.label7.Text = "Plat:";
            this.btnAddIso.Location = new Point(0xde, 0x16c);
            this.btnAddIso.Name = "btnAddIso";
            this.btnAddIso.Size = new Size(0x58, 0x17);
            this.btnAddIso.TabIndex = 15;
            this.btnAddIso.Text = "Add ISO";
            this.btnAddIso.UseVisualStyleBackColor = true;
            this.btnAddIso.Click += new EventHandler(this.button2_Click);
            this.btnCancel.Location = new Point(0x13c, 0x16c);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new Size(0x4b, 0x17);
            this.btnCancel.TabIndex = 0x10;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new EventHandler(this.button1_Click);
            this.ttSettings.AutoPopDelay = 0x2710;
            this.ttSettings.InitialDelay = 100;
            this.ttSettings.IsBalloon = true;
            this.ttSettings.ReshowDelay = 100;
            this.ttThumb.AutomaticDelay = 10;
            this.ttThumb.AutoPopDelay = 0x1388;
            this.ttThumb.InitialDelay = 10;
            this.ttThumb.IsBalloon = true;
            this.ttThumb.ReshowDelay = 2;
            this.groupBox1.BackColor = SystemColors.Control;
            this.groupBox1.Controls.Add(this.cbSaveRebuilt);
            this.groupBox1.Controls.Add(this.btnRebuiltBrowse);
            this.groupBox1.Controls.Add(this.cmbPaddingMode);
            this.groupBox1.Controls.Add(this.label12);
            this.groupBox1.Controls.Add(this.label14);
            this.groupBox1.Controls.Add(this.txtRebuiltIso);
            this.groupBox1.Controls.Add(this.pbPadding);
            this.groupBox1.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.groupBox1.ForeColor = SystemColors.ControlText;
            this.groupBox1.Location = new Point(13, 0xfc);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new Size(0x183, 0x68);
            this.groupBox1.TabIndex = 0x29;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Padding Removal";
            this.cbSaveRebuilt.AutoSize = true;
            this.cbSaveRebuilt.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.cbSaveRebuilt.Location = new Point(0xe0, 0x17);
            this.cbSaveRebuilt.Name = "cbSaveRebuilt";
            this.cbSaveRebuilt.Size = new Size(0x95, 0x11);
            this.cbSaveRebuilt.TabIndex = 12;
            this.cbSaveRebuilt.Text = "Save Rebuilt ISO Image?";
            this.cbSaveRebuilt.UseVisualStyleBackColor = true;
            this.cbSaveRebuilt.CheckedChanged += new EventHandler(this.cbSaveRebuilt_CheckedChanged);
            this.btnRebuiltBrowse.Enabled = false;
            this.btnRebuiltBrowse.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.btnRebuiltBrowse.Location = new Point(0x12f, 0x43);
            this.btnRebuiltBrowse.Name = "btnRebuiltBrowse";
            this.btnRebuiltBrowse.Size = new Size(0x4b, 0x17);
            this.btnRebuiltBrowse.TabIndex = 14;
            this.btnRebuiltBrowse.Text = "&Browse";
            this.btnRebuiltBrowse.UseVisualStyleBackColor = true;
            this.btnRebuiltBrowse.Click += new EventHandler(this.btnRebuiltBrowse_Click);
            this.cmbPaddingMode.DropDownStyle = ComboBoxStyle.DropDownList;
            this.cmbPaddingMode.FormattingEnabled = true;
            this.cmbPaddingMode.Items.AddRange(new object[] { "None", "Partial (ISO Cropped)", "Full (ISO Rebuilt)" });
            this.cmbPaddingMode.Location = new Point(0x43, 0x15);
            this.cmbPaddingMode.Name = "cmbPaddingMode";
            this.cmbPaddingMode.Size = new Size(0x88, 0x15);
            this.cmbPaddingMode.TabIndex = 11;
            this.cmbPaddingMode.SelectedIndexChanged += new EventHandler(this.cmbPaddingMode_SelectedIndexChanged);
            this.label12.AutoSize = true;
            this.label12.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label12.Location = new Point(6, 0x18);
            this.label12.Name = "label12";
            this.label12.Size = new Size(40, 13);
            this.label12.TabIndex = 0x1f;
            this.label12.Text = "Mode:";
            this.label14.AutoSize = true;
            this.label14.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label14.Location = new Point(6, 0x31);
            this.label14.Name = "label14";
            this.label14.Size = new Size(0xbd, 13);
            this.label14.TabIndex = 0x1d;
            this.label14.Text = "Temporary Location for Rebuilt ISO:";
            this.txtRebuiltIso.Enabled = false;
            this.txtRebuiltIso.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtRebuiltIso.Location = new Point(9, 0x44);
            this.txtRebuiltIso.Name = "txtRebuiltIso";
            this.txtRebuiltIso.Size = new Size(0x120, 0x16);
            this.txtRebuiltIso.TabIndex = 13;
            this.pbPadding.BackgroundImage = Resources.Hint;
            this.pbPadding.BackgroundImageLayout = ImageLayout.Zoom;
            this.pbPadding.Location = new Point(0x6a, 0);
            this.pbPadding.Name = "pbPadding";
            this.pbPadding.Size = new Size(15, 15);
            this.pbPadding.TabIndex = 0x19;
            this.pbPadding.TabStop = false;
            this.ttPadding.AutoPopDelay = 0x2710;
            this.ttPadding.InitialDelay = 100;
            this.ttPadding.IsBalloon = true;
            this.ttPadding.ReshowDelay = 100;
            base.AutoScaleDimensions = new SizeF(6f, 13f);
            base.AutoScaleMode = AutoScaleMode.Font;
            base.ClientSize = new Size(0x19c, 0x18e);
            base.Controls.Add(this.groupBox1);
            base.Controls.Add(this.btnAddIso);
            base.Controls.Add(this.btnCancel);
            base.Controls.Add(this.groupBox3);
            base.Controls.Add(this.groupBox2);
            base.FormBorderStyle = FormBorderStyle.FixedDialog;
            base.MaximizeBox = false;
            base.MinimizeBox = false;
            base.Name = "AddISO";
            base.StartPosition = FormStartPosition.CenterParent;
            this.Text = "Add ISO Image";
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((ISupportInitialize) this.pbVideo).EndInit();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            ((ISupportInitialize) this.pbThumb).EndInit();
            ((ISupportInitialize) this.pbTime).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((ISupportInitialize) this.pbPadding).EndInit();
            base.ResumeLayout(false);
        }

        private void isoDetails_ProgressChanged(object sender, ProgressChangedEventArgs e)
        {
            IsoDetailsResults userState = (IsoDetailsResults) e.UserState;
            if (userState.Results == IsoDetailsResultsType.Error)
            {
                MessageBox.Show(userState.ErrorMessage, "Error Reading Title Information");
            }
            else
            {
                this.txtName.Text = userState.ProgressMessage;
            }
        }

        private void isoDetails_RunWorkerCompleted(object sender, RunWorkerCompletedEventArgs e)
        {
            if (e.Result == null)
            {
                this.txtName.Text = "Failed to read details from ISO image.";
            }
            else
            {
                IsoDetailsResults result = (IsoDetailsResults) e.Result;
                switch (result.ConsolePlatform)
                {
                    case IsoDetailsPlatform.Xbox:
                        this.platform = IsoEntryPlatform.Xbox;
                        break;

                    case IsoDetailsPlatform.Xbox360:
                        this.platform = IsoEntryPlatform.Xbox360;
                        break;
                }
                bool flag = (bool) Chilano.Iso2God.Properties.Settings.Default["AutoRenameMultiDisc"];
                int num = 0;
                int.TryParse(result.DiscCount, out num);
                this.txtName.Text = (flag && (num > 1)) ? (result.Name + " Disc " + result.DiscNumber) : result.Name;
                this.txtTitleID.Text = result.TitleID;
                this.txtMediaID.Text = result.MediaID;
                this.txtPlatform.Text = result.Platform;
                this.txtExType.Text = result.ExType;
                this.txtDiscNum.Text = result.DiscNumber;
                this.txtDiscCount.Text = result.DiscCount;
                if ((result.Thumbnail != null) && (result.RawThumbnail != null))
                {
                    this.pbThumb.Image = (Image) result.Thumbnail.Clone();
                    this.pbThumb.Tag = (byte[]) result.RawThumbnail.Clone();
                }
            }
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {
            OpenFileDialog dialog = new OpenFileDialog {
                InitialDirectory = Environment.SpecialFolder.Desktop.ToString(),
                Title = "Choose an image to use as the Title Thumbnail:",
                Filter = "Supported Images (*.png, *.jpg, *.bmp)|*.png;*.jpg;*.bmp",
                Multiselect = false
            };
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                if (!File.Exists(dialog.FileName))
                {
                    MessageBox.Show("Could not locate specified thumbnail.");
                }
                else
                {
                    byte[] buffer = File.ReadAllBytes(dialog.FileName);
                    Image image = Image.FromFile(dialog.FileName);
                    Image image2 = new Bitmap(0x40, 0x40);
                    if ((image.Width > 0x40) || (image.Height > 0x40))
                    {
                        Graphics.FromImage(image2).DrawImage(image, 0, 0, 0x40, 0x40);
                        this.pbThumb.Image = (Image) image2.Clone();
                        MemoryStream stream = new MemoryStream();
                        image2.Save(stream, ImageFormat.Png);
                        this.pbThumb.Tag = stream.ToArray();
                    }
                    else
                    {
                        this.pbThumb.Image = (Image) image.Clone();
                        this.pbThumb.Tag = buffer;
                    }
                }
            }
        }
    }
}

