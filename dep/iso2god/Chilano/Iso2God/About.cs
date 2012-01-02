namespace Chilano.Iso2God
{
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.Windows.Forms;

    public class About : Form
    {
        private Button btnClose;
        private IContainer components;
        private Label label1;
        private Label label2;
        private Label lblVersion;
        private PictureBox pictureBox1;

        public About()
        {
            this.InitializeComponent();
            this.lblVersion.BackColor = Color.Transparent;
            this.lblVersion.Parent = this.pictureBox1;
        }

        private void About_Load(object sender, EventArgs e)
        {
            this.lblVersion.Text = "Version: " + ((Main) base.Owner).getVersion(true, false);
        }

        private void button1_Click(object sender, EventArgs e)
        {
            base.Close();
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
            ComponentResourceManager manager = new ComponentResourceManager(typeof(About));
            this.lblVersion = new Label();
            this.btnClose = new Button();
            this.label1 = new Label();
            this.pictureBox1 = new PictureBox();
            this.label2 = new Label();
            ((ISupportInitialize) this.pictureBox1).BeginInit();
            base.SuspendLayout();
            this.lblVersion.BackColor = Color.White;
            this.lblVersion.Font = new Font("Segoe UI", 8.25f, FontStyle.Bold, GraphicsUnit.Point, 0);
            this.lblVersion.Location = new Point(0x165, 8);
            this.lblVersion.Name = "lblVersion";
            this.lblVersion.Size = new Size(0x7a, 0x10);
            this.lblVersion.TabIndex = 1;
            this.lblVersion.Text = "label1";
            this.lblVersion.TextAlign = ContentAlignment.MiddleRight;
            this.btnClose.DialogResult = DialogResult.Cancel;
            this.btnClose.Location = new Point(400, 0x100);
            this.btnClose.Name = "btnClose";
            this.btnClose.Size = new Size(0x4b, 0x17);
            this.btnClose.TabIndex = 0;
            this.btnClose.Text = "Close";
            this.btnClose.UseVisualStyleBackColor = true;
            this.btnClose.Click += new EventHandler(this.button1_Click);
            this.label1.BackColor = SystemColors.Control;
            this.label1.Location = new Point(12, 0x6a);
            this.label1.Name = "label1";
            this.label1.Size = new Size(0x1cf, 0x8a);
            this.label1.TabIndex = 4;
            this.label1.Text = manager.GetString("label1.Text");
            this.pictureBox1.BackColor = Color.White;
            this.pictureBox1.BackgroundImage = (Image) manager.GetObject("pictureBox1.BackgroundImage");
            this.pictureBox1.BackgroundImageLayout = ImageLayout.Center;
            this.pictureBox1.BorderStyle = BorderStyle.FixedSingle;
            this.pictureBox1.Cursor = Cursors.Hand;
            this.pictureBox1.Location = new Point(-1, -1);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new Size(0x1ec, 0x5c);
            this.pictureBox1.TabIndex = 0;
            this.pictureBox1.TabStop = false;
            this.pictureBox1.Click += new EventHandler(this.pictureBox1_Click);
            this.label2.Location = new Point(12, 0xf4);
            this.label2.Name = "label2";
            this.label2.Size = new Size(0x17e, 0x23);
            this.label2.TabIndex = 9;
            this.label2.Text = "Thanks go out to rolly poly, MikeJJ, Icekiller, Razkar, dstruktiv and threesixtyuser for your help. ";
            base.AcceptButton = this.btnClose;
            base.AutoScaleDimensions = new SizeF(6f, 13f);
            base.AutoScaleMode = AutoScaleMode.Font;
            this.BackColor = SystemColors.Control;
            base.CancelButton = this.btnClose;
            base.ClientSize = new Size(0x1e7, 0x123);
            base.Controls.Add(this.label2);
            base.Controls.Add(this.btnClose);
            base.Controls.Add(this.lblVersion);
            base.Controls.Add(this.pictureBox1);
            base.Controls.Add(this.label1);
            this.Font = new Font("Segoe UI", 8.25f, FontStyle.Regular, GraphicsUnit.Point, 0);
            base.FormBorderStyle = FormBorderStyle.FixedDialog;
            base.Icon = (Icon) manager.GetObject("$this.Icon");
            base.MaximizeBox = false;
            base.MinimizeBox = false;
            base.Name = "About";
            base.StartPosition = FormStartPosition.CenterParent;
            this.Text = "About Iso2God";
            base.Load += new EventHandler(this.About_Load);
            ((ISupportInitialize) this.pictureBox1).EndInit();
            base.ResumeLayout(false);
        }

        private void linkLabel1_LinkClicked(object sender, LinkLabelLinkClickedEventArgs e)
        {
        }

        private void pictureBox1_Click(object sender, EventArgs e)
        {
        }

        private void pictureBox2_Click(object sender, EventArgs e)
        {
        }
    }
}

