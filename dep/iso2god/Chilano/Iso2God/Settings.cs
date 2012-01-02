namespace Chilano.Iso2God
{
    using Chilano.Iso2God.Properties;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.IO;
    using System.Windows.Forms;

    public class Settings : Form
    {
        private Button btnCancel;
        private Button btnOutBrowse;
        private Button btnRebuild;
        private Button btnSave;
        private CheckBox cbAlwaysSave;
        private CheckBox cbAutoBrowse;
        private CheckBox cbAutoRename;
        private CheckBox cbFTP;
        private CheckBox cbRebuildCheck;
        private ComboBox cmbPadding;
        private IContainer components;
        private GroupBox groupBox1;
        private GroupBox groupBox2;
        private GroupBox groupBox3;
        private Label label1;
        private Label label2;
        private Label label3;
        private Label label4;
        private Label label5;
        private Label label6;
        private Label label7;
        private PictureBox pbFTP;
        private PictureBox pbOptions;
        private PictureBox pbRipping;
        private ToolTip ttFTP;
        private ToolTip ttOptions;
        private ToolTip ttSettings;
        private TextBox txtFtpIp;
        private TextBox txtFtpPass;
        private TextBox txtFtpPort;
        private TextBox txtFtpUser;
        private TextBox txtOut;
        private TextBox txtRebuild;

        public Settings()
        {
            this.InitializeComponent();
            this.loadSettings();
            this.ttSettings.SetToolTip(this.pbRipping, "Output Path allows you to set a default path for all GODs to be stored in. Each GOD will be stored\nin a sub-directory using the TitleID of the default.xex stored in the ISO.\n\nRebuild Path allows you to specify a default path to store rebuilt ISO images.");
            this.ttFTP.SetToolTip(this.pbFTP, "Once an ISO image has been converted to a GOD container, it can be automatically\nuploaded to your Xbox 360 using FTP (if you are running a dashboard such as XexMenu\nwith a built-in FTP server).");
            this.ttOptions.SetToolTip(this.pbOptions, "To change the behaviour of Iso2God, you can change the options below.");
        }

        private void btnCancel_Click(object sender, EventArgs e)
        {
            base.Close();
        }

        private void btnDestBrowse_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog {
                RootFolder = Environment.SpecialFolder.Desktop,
                ShowNewFolderButton = true,
                Description = "Choose where to store your GOD containers:"
            };
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                this.txtOut.Text = dialog.SelectedPath;
                if (!this.txtOut.Text.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    this.txtOut.Text = this.txtOut.Text + Path.DirectorySeparatorChar;
                }
            }
        }

        private void btnRebuild_Click(object sender, EventArgs e)
        {
            FolderBrowserDialog dialog = new FolderBrowserDialog {
                RootFolder = Environment.SpecialFolder.Desktop,
                ShowNewFolderButton = true,
                Description = "Choose where to store rebuilt ISO images:"
            };
            if (dialog.ShowDialog() == DialogResult.OK)
            {
                this.txtRebuild.Text = dialog.SelectedPath;
                if (!this.txtRebuild.Text.EndsWith(Path.DirectorySeparatorChar.ToString()))
                {
                    this.txtRebuild.Text = this.txtRebuild.Text + Path.DirectorySeparatorChar;
                }
            }
        }

        private void btnSave_Click(object sender, EventArgs e)
        {
            Chilano.Iso2God.Properties.Settings.Default["OutputPath"] = this.txtOut.Text;
            Chilano.Iso2God.Properties.Settings.Default["RebuildPath"] = this.txtRebuild.Text;
            Chilano.Iso2God.Properties.Settings.Default["RebuiltCheck"] = this.cbRebuildCheck.Checked;
            Chilano.Iso2God.Properties.Settings.Default["AlwaysSave"] = this.cbAlwaysSave.Checked;
            Chilano.Iso2God.Properties.Settings.Default["AutoRenameMultiDisc"] = this.cbAutoRename.Checked;
            Chilano.Iso2God.Properties.Settings.Default["AutoBrowse"] = this.cbAutoBrowse.Checked;
            Chilano.Iso2God.Properties.Settings.Default["FtpUpload"] = this.cbFTP.Checked;
            Chilano.Iso2God.Properties.Settings.Default["FtpIP"] = this.txtFtpIp.Text;
            Chilano.Iso2God.Properties.Settings.Default["FtpUser"] = this.txtFtpUser.Text;
            Chilano.Iso2God.Properties.Settings.Default["FtpPass"] = this.txtFtpPass.Text;
            Chilano.Iso2God.Properties.Settings.Default["FtpPort"] = this.txtFtpPort.Text;
            Chilano.Iso2God.Properties.Settings.Default["DefaultPadding"] = this.cmbPadding.SelectedIndex;
            Chilano.Iso2God.Properties.Settings.Default.Save();
            (base.Owner as Main).UpdateSpace();
            base.Close();
        }

        private void cbFTP_CheckedChanged(object sender, EventArgs e)
        {
            this.txtFtpIp.Enabled = this.cbFTP.Checked;
            this.txtFtpUser.Enabled = this.cbFTP.Checked;
            this.txtFtpPass.Enabled = this.cbFTP.Checked;
            this.txtFtpPort.Enabled = this.cbFTP.Checked;
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && (this.components != null))
            {
                this.components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void InitializeComponent()
        {
            this.components = new Container();
            ComponentResourceManager manager = new ComponentResourceManager(typeof(Chilano.Iso2God.Settings));
            this.groupBox2 = new GroupBox();
            this.btnRebuild = new Button();
            this.txtRebuild = new TextBox();
            this.label1 = new Label();
            this.pbRipping = new PictureBox();
            this.btnOutBrowse = new Button();
            this.txtOut = new TextBox();
            this.label3 = new Label();
            this.btnSave = new Button();
            this.btnCancel = new Button();
            this.ttSettings = new ToolTip(this.components);
            this.groupBox1 = new GroupBox();
            this.cmbPadding = new ComboBox();
            this.label6 = new Label();
            this.cbAutoRename = new CheckBox();
            this.cbAlwaysSave = new CheckBox();
            this.cbRebuildCheck = new CheckBox();
            this.pbOptions = new PictureBox();
            this.groupBox3 = new GroupBox();
            this.txtFtpPort = new TextBox();
            this.label7 = new Label();
            this.cbFTP = new CheckBox();
            this.txtFtpPass = new TextBox();
            this.label5 = new Label();
            this.txtFtpUser = new TextBox();
            this.label2 = new Label();
            this.pbFTP = new PictureBox();
            this.txtFtpIp = new TextBox();
            this.label4 = new Label();
            this.ttFTP = new ToolTip(this.components);
            this.ttOptions = new ToolTip(this.components);
            this.cbAutoBrowse = new CheckBox();
            this.groupBox2.SuspendLayout();
            ((ISupportInitialize) this.pbRipping).BeginInit();
            this.groupBox1.SuspendLayout();
            ((ISupportInitialize) this.pbOptions).BeginInit();
            this.groupBox3.SuspendLayout();
            ((ISupportInitialize) this.pbFTP).BeginInit();
            base.SuspendLayout();
            this.groupBox2.BackColor = SystemColors.Control;
            this.groupBox2.Controls.Add(this.btnRebuild);
            this.groupBox2.Controls.Add(this.txtRebuild);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Controls.Add(this.pbRipping);
            this.groupBox2.Controls.Add(this.btnOutBrowse);
            this.groupBox2.Controls.Add(this.txtOut);
            this.groupBox2.Controls.Add(this.label3);
            this.groupBox2.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.groupBox2.ForeColor = SystemColors.ControlText;
            this.groupBox2.Location = new Point(12, 12);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new Size(0x1be, 80);
            this.groupBox2.TabIndex = 0x16;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Location Settings";
            this.btnRebuild.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.btnRebuild.Location = new Point(0x16d, 0x2e);
            this.btnRebuild.Name = "btnRebuild";
            this.btnRebuild.Size = new Size(0x4b, 0x17);
            this.btnRebuild.TabIndex = 0x1c;
            this.btnRebuild.Text = "&Browse";
            this.btnRebuild.UseVisualStyleBackColor = true;
            this.btnRebuild.Click += new EventHandler(this.btnRebuild_Click);
            this.txtRebuild.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtRebuild.Location = new Point(0x54, 0x2e);
            this.txtRebuild.Name = "txtRebuild";
            this.txtRebuild.Size = new Size(0x113, 0x16);
            this.txtRebuild.TabIndex = 0x1b;
            this.label1.AutoSize = true;
            this.label1.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label1.Location = new Point(6, 0x31);
            this.label1.Name = "label1";
            this.label1.Size = new Size(0x4c, 13);
            this.label1.TabIndex = 0x1d;
            this.label1.Text = "Rebuild Path:";
            this.pbRipping.BackgroundImage = Resources.Hint;
            this.pbRipping.BackgroundImageLayout = ImageLayout.Zoom;
            this.pbRipping.Location = new Point(0x69, 0);
            this.pbRipping.Name = "pbRipping";
            this.pbRipping.Size = new Size(15, 15);
            this.pbRipping.TabIndex = 0x1a;
            this.pbRipping.TabStop = false;
            this.btnOutBrowse.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.btnOutBrowse.Location = new Point(0x16d, 0x11);
            this.btnOutBrowse.Name = "btnOutBrowse";
            this.btnOutBrowse.Size = new Size(0x4b, 0x17);
            this.btnOutBrowse.TabIndex = 2;
            this.btnOutBrowse.Text = "&Browse";
            this.btnOutBrowse.UseVisualStyleBackColor = true;
            this.btnOutBrowse.Click += new EventHandler(this.btnDestBrowse_Click);
            this.txtOut.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtOut.Location = new Point(0x54, 0x12);
            this.txtOut.Name = "txtOut";
            this.txtOut.Size = new Size(0x113, 0x16);
            this.txtOut.TabIndex = 0;
            this.label3.AutoSize = true;
            this.label3.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label3.Location = new Point(6, 0x15);
            this.label3.Name = "label3";
            this.label3.Size = new Size(0x4a, 13);
            this.label3.TabIndex = 10;
            this.label3.Text = "Output Path:";
            this.btnSave.Location = new Point(0x11b, 0xf9);
            this.btnSave.Name = "btnSave";
            this.btnSave.Size = new Size(0x58, 0x17);
            this.btnSave.TabIndex = 4;
            this.btnSave.Text = "Save Changes";
            this.btnSave.UseVisualStyleBackColor = true;
            this.btnSave.Click += new EventHandler(this.btnSave_Click);
            this.btnCancel.Location = new Point(0x179, 0xf9);
            this.btnCancel.Name = "btnCancel";
            this.btnCancel.Size = new Size(0x4b, 0x17);
            this.btnCancel.TabIndex = 5;
            this.btnCancel.Text = "Cancel";
            this.btnCancel.UseVisualStyleBackColor = true;
            this.btnCancel.Click += new EventHandler(this.btnCancel_Click);
            this.ttSettings.AutoPopDelay = 0x2710;
            this.ttSettings.InitialDelay = 100;
            this.ttSettings.IsBalloon = true;
            this.ttSettings.ReshowDelay = 100;
            this.groupBox1.BackColor = SystemColors.Control;
            this.groupBox1.Controls.Add(this.cbAutoBrowse);
            this.groupBox1.Controls.Add(this.cmbPadding);
            this.groupBox1.Controls.Add(this.label6);
            this.groupBox1.Controls.Add(this.cbAutoRename);
            this.groupBox1.Controls.Add(this.cbAlwaysSave);
            this.groupBox1.Controls.Add(this.cbRebuildCheck);
            this.groupBox1.Controls.Add(this.pbOptions);
            this.groupBox1.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.groupBox1.ForeColor = SystemColors.ControlText;
            this.groupBox1.Location = new Point(12, 0x62);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new Size(220, 0x91);
            this.groupBox1.TabIndex = 30;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Options";
            this.cmbPadding.DropDownStyle = ComboBoxStyle.DropDownList;
            this.cmbPadding.FormattingEnabled = true;
            this.cmbPadding.Items.AddRange(new object[] { "None", "Partial (ISO Cropped)", "Full (ISO Rebuilt)" });
            this.cmbPadding.Location = new Point(0x51, 0x71);
            this.cmbPadding.Name = "cmbPadding";
            this.cmbPadding.Size = new Size(0x85, 0x15);
            this.cmbPadding.TabIndex = 0x20;
            this.label6.AutoSize = true;
            this.label6.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label6.Location = new Point(6, 0x74);
            this.label6.Name = "label6";
            this.label6.Size = new Size(0x35, 13);
            this.label6.TabIndex = 30;
            this.label6.Text = "Padding:";
            this.cbAutoRename.AutoSize = true;
            this.cbAutoRename.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.cbAutoRename.Location = new Point(9, 0x43);
            this.cbAutoRename.Name = "cbAutoRename";
            this.cbAutoRename.Size = new Size(0xb6, 0x11);
            this.cbAutoRename.TabIndex = 0x1d;
            this.cbAutoRename.Text = "Auto-rename multi-disc games";
            this.cbAutoRename.UseVisualStyleBackColor = true;
            this.cbAlwaysSave.AutoSize = true;
            this.cbAlwaysSave.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.cbAlwaysSave.Location = new Point(9, 0x15);
            this.cbAlwaysSave.Name = "cbAlwaysSave";
            this.cbAlwaysSave.Size = new Size(0x90, 0x11);
            this.cbAlwaysSave.TabIndex = 0x1c;
            this.cbAlwaysSave.Text = "Always save rebuilt ISO";
            this.cbAlwaysSave.UseVisualStyleBackColor = true;
            this.cbRebuildCheck.AutoSize = true;
            this.cbRebuildCheck.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.cbRebuildCheck.Location = new Point(9, 0x2c);
            this.cbRebuildCheck.Name = "cbRebuildCheck";
            this.cbRebuildCheck.Size = new Size(0xc7, 0x11);
            this.cbRebuildCheck.TabIndex = 0x1b;
            this.cbRebuildCheck.Text = "Ask if rebuilt ISO should be saved";
            this.cbRebuildCheck.UseVisualStyleBackColor = true;
            this.pbOptions.BackgroundImage = Resources.Hint;
            this.pbOptions.BackgroundImageLayout = ImageLayout.Zoom;
            this.pbOptions.Location = new Point(0x37, 0);
            this.pbOptions.Name = "pbOptions";
            this.pbOptions.Size = new Size(15, 15);
            this.pbOptions.TabIndex = 0x1a;
            this.pbOptions.TabStop = false;
            this.groupBox3.BackColor = SystemColors.Control;
            this.groupBox3.Controls.Add(this.txtFtpPort);
            this.groupBox3.Controls.Add(this.label7);
            this.groupBox3.Controls.Add(this.cbFTP);
            this.groupBox3.Controls.Add(this.txtFtpPass);
            this.groupBox3.Controls.Add(this.label5);
            this.groupBox3.Controls.Add(this.txtFtpUser);
            this.groupBox3.Controls.Add(this.label2);
            this.groupBox3.Controls.Add(this.pbFTP);
            this.groupBox3.Controls.Add(this.txtFtpIp);
            this.groupBox3.Controls.Add(this.label4);
            this.groupBox3.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.groupBox3.ForeColor = SystemColors.ControlText;
            this.groupBox3.Location = new Point(0xee, 0x62);
            this.groupBox3.Name = "groupBox3";
            this.groupBox3.Size = new Size(220, 0x91);
            this.groupBox3.TabIndex = 0x1f;
            this.groupBox3.TabStop = false;
            this.groupBox3.Text = "FTP Transfer";
            this.txtFtpPort.Enabled = false;
            this.txtFtpPort.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtFtpPort.Location = new Point(0x4e, 0x69);
            this.txtFtpPort.Name = "txtFtpPort";
            this.txtFtpPort.Size = new Size(0x85, 0x16);
            this.txtFtpPort.TabIndex = 0x21;
            this.label7.AutoSize = true;
            this.label7.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label7.Location = new Point(5, 0x6c);
            this.label7.Name = "label7";
            this.label7.Size = new Size(0x1f, 13);
            this.label7.TabIndex = 0x20;
            this.label7.Text = "Port:";
            this.cbFTP.AutoSize = true;
            this.cbFTP.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.cbFTP.Location = new Point(0x99, 0);
            this.cbFTP.Name = "cbFTP";
            this.cbFTP.Size = new Size(0x3d, 0x11);
            this.cbFTP.TabIndex = 30;
            this.cbFTP.Text = "Enable";
            this.cbFTP.UseVisualStyleBackColor = true;
            this.cbFTP.CheckedChanged += new EventHandler(this.cbFTP_CheckedChanged);
            this.txtFtpPass.Enabled = false;
            this.txtFtpPass.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtFtpPass.Location = new Point(0x4e, 0x4d);
            this.txtFtpPass.Name = "txtFtpPass";
            this.txtFtpPass.Size = new Size(0x85, 0x16);
            this.txtFtpPass.TabIndex = 30;
            this.label5.AutoSize = true;
            this.label5.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label5.Location = new Point(5, 80);
            this.label5.Name = "label5";
            this.label5.Size = new Size(0x3b, 13);
            this.label5.TabIndex = 0x1f;
            this.label5.Text = "Password:";
            this.txtFtpUser.Enabled = false;
            this.txtFtpUser.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtFtpUser.Location = new Point(0x4e, 0x31);
            this.txtFtpUser.Name = "txtFtpUser";
            this.txtFtpUser.Size = new Size(0x85, 0x16);
            this.txtFtpUser.TabIndex = 0x1b;
            this.label2.AutoSize = true;
            this.label2.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label2.Location = new Point(5, 0x34);
            this.label2.Name = "label2";
            this.label2.Size = new Size(0x3d, 13);
            this.label2.TabIndex = 0x1d;
            this.label2.Text = "Username:";
            this.pbFTP.BackgroundImage = Resources.Hint;
            this.pbFTP.BackgroundImageLayout = ImageLayout.Zoom;
            this.pbFTP.Location = new Point(0x51, 0);
            this.pbFTP.Name = "pbFTP";
            this.pbFTP.Size = new Size(15, 15);
            this.pbFTP.TabIndex = 0x1a;
            this.pbFTP.TabStop = false;
            this.txtFtpIp.Enabled = false;
            this.txtFtpIp.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.txtFtpIp.Location = new Point(0x4e, 0x15);
            this.txtFtpIp.Name = "txtFtpIp";
            this.txtFtpIp.Size = new Size(0x85, 0x16);
            this.txtFtpIp.TabIndex = 0;
            this.label4.AutoSize = true;
            this.label4.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.label4.Location = new Point(5, 0x18);
            this.label4.Name = "label4";
            this.label4.Size = new Size(0x3f, 13);
            this.label4.TabIndex = 10;
            this.label4.Text = "IP Address:";
            this.ttFTP.AutoPopDelay = 0x2710;
            this.ttFTP.InitialDelay = 100;
            this.ttFTP.IsBalloon = true;
            this.ttFTP.ReshowDelay = 100;
            this.ttOptions.AutoPopDelay = 0x2710;
            this.ttOptions.InitialDelay = 100;
            this.ttOptions.IsBalloon = true;
            this.ttOptions.ReshowDelay = 100;
            this.cbAutoBrowse.AutoSize = true;
            this.cbAutoBrowse.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            this.cbAutoBrowse.Location = new Point(9, 90);
            this.cbAutoBrowse.Name = "cbAutoBrowse";
            this.cbAutoBrowse.Size = new Size(0xba, 0x11);
            this.cbAutoBrowse.TabIndex = 0x21;
            this.cbAutoBrowse.Text = "Auto-browse when adding ISO";
            this.cbAutoBrowse.UseVisualStyleBackColor = true;
            base.AutoScaleDimensions = new SizeF(6f, 13f);
            base.AutoScaleMode = AutoScaleMode.Font;
            base.ClientSize = new Size(470, 280);
            base.Controls.Add(this.groupBox3);
            base.Controls.Add(this.groupBox1);
            base.Controls.Add(this.btnSave);
            base.Controls.Add(this.btnCancel);
            base.Controls.Add(this.groupBox2);
            this.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            base.FormBorderStyle = FormBorderStyle.FixedDialog;
            base.Icon = (Icon) manager.GetObject("$this.Icon");
            base.MaximizeBox = false;
            base.MinimizeBox = false;
            base.Name = "Settings";
            base.StartPosition = FormStartPosition.CenterParent;
            this.Text = "Iso2God Settings";
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            ((ISupportInitialize) this.pbRipping).EndInit();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((ISupportInitialize) this.pbOptions).EndInit();
            this.groupBox3.ResumeLayout(false);
            this.groupBox3.PerformLayout();
            ((ISupportInitialize) this.pbFTP).EndInit();
            base.ResumeLayout(false);
        }

        private void loadSettings()
        {
            try
            {
                this.txtOut.Text = Chilano.Iso2God.Properties.Settings.Default["OutputPath"].ToString();
                this.txtRebuild.Text = Chilano.Iso2God.Properties.Settings.Default["RebuildPath"].ToString();
                this.cbRebuildCheck.Checked = (bool) Chilano.Iso2God.Properties.Settings.Default["RebuiltCheck"];
                this.cbAlwaysSave.Checked = (bool) Chilano.Iso2God.Properties.Settings.Default["AlwaysSave"];
                this.cbAutoRename.Checked = (bool) Chilano.Iso2God.Properties.Settings.Default["AutoRenameMultiDisc"];
                this.cbAutoBrowse.Checked = (bool) Chilano.Iso2God.Properties.Settings.Default["AutoBrowse"];
                this.cbFTP.Checked = (bool) Chilano.Iso2God.Properties.Settings.Default["FtpUpload"];
                this.txtFtpIp.Text = Chilano.Iso2God.Properties.Settings.Default["FtpIP"].ToString();
                this.txtFtpUser.Text = Chilano.Iso2God.Properties.Settings.Default["FtpUser"].ToString();
                this.txtFtpPass.Text = Chilano.Iso2God.Properties.Settings.Default["FtpPass"].ToString();
                this.txtFtpPort.Text = Chilano.Iso2God.Properties.Settings.Default["FtpPort"].ToString();
                this.cmbPadding.SelectedIndex = (int) Chilano.Iso2God.Properties.Settings.Default["DefaultPadding"];
            }
            catch
            {
                MessageBox.Show("Unable to load User Settings.\n\nIf this is the first time you've seen this message, the problem will be resolved when you save your settings.\n\nIf you've seen it before, please contact the author about this issue.");
            }
        }
    }
}

