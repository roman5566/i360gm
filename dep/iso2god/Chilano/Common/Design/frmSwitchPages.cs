namespace Chilano.Common.Design
{
    using Chilano.Common;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.Windows.Forms;

    public class frmSwitchPages : Form
    {
        private IContainer components = null;
        private Button myCtlBtnCancel;
        private Button myCtlBtnOK;
        private CheckBox myCtlChkSetSelectedPage;
        private ComboBox myCtlCmbItems;
        private Label myCtlLblSwitchPage;
        private MultiPaneControlDesigner myDesigner;
        private MultiPanePage myFutureSelectedItem;
        private bool mySetSelectedPage;

        public frmSwitchPages(MultiPaneControlDesigner theDesigner)
        {
            this.myDesigner = theDesigner;
            this.InitializeComponent();
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing && (this.components != null))
            {
                this.components.Dispose();
            }
            base.Dispose(disposing);
        }

        private void Handler_OK(object sender, EventArgs e)
        {
            this.myFutureSelectedItem = ((MultiPanePageItem) this.myCtlCmbItems.SelectedItem).Page;
            this.mySetSelectedPage = this.myCtlChkSetSelectedPage.Checked;
        }

        private void InitializeComponent()
        {
            this.myCtlLblSwitchPage = new Label();
            this.myCtlCmbItems = new ComboBox();
            this.myCtlChkSetSelectedPage = new CheckBox();
            this.myCtlBtnOK = new Button();
            this.myCtlBtnCancel = new Button();
            base.SuspendLayout();
            this.myCtlLblSwitchPage.AutoSize = true;
            this.myCtlLblSwitchPage.Location = new Point(9, 9);
            this.myCtlLblSwitchPage.Name = "myCtlLblSwitchPage";
            this.myCtlLblSwitchPage.TabIndex = 0;
            this.myCtlLblSwitchPage.Text = "Switch the page to:";
            this.myCtlCmbItems.DropDownStyle = ComboBoxStyle.DropDownList;
            this.myCtlCmbItems.Location = new Point(12, 0x19);
            this.myCtlCmbItems.Name = "myCtlCmbItems";
            this.myCtlCmbItems.Size = new Size(0xe3, 0x15);
            this.myCtlCmbItems.TabIndex = 1;
            this.myCtlChkSetSelectedPage.Location = new Point(12, 0x3d);
            this.myCtlChkSetSelectedPage.Name = "myCtlChkSetSelectedPage";
            this.myCtlChkSetSelectedPage.Size = new Size(220, 0x11);
            this.myCtlChkSetSelectedPage.TabIndex = 2;
            this.myCtlChkSetSelectedPage.Text = "Also set the SelectedPage property";
            this.myCtlBtnOK.Anchor = AnchorStyles.Right | AnchorStyles.Bottom;
            this.myCtlBtnOK.DialogResult = DialogResult.OK;
            this.myCtlBtnOK.Location = new Point(0x4d, 0x61);
            this.myCtlBtnOK.Name = "myCtlBtnOK";
            this.myCtlBtnOK.TabIndex = 3;
            this.myCtlBtnOK.Text = "OK";
            this.myCtlBtnOK.Click += new EventHandler(this.Handler_OK);
            this.myCtlBtnCancel.Anchor = AnchorStyles.Right | AnchorStyles.Bottom;
            this.myCtlBtnCancel.DialogResult = DialogResult.Cancel;
            this.myCtlBtnCancel.Location = new Point(0xa4, 0x61);
            this.myCtlBtnCancel.Name = "myCtlBtnCancel";
            this.myCtlBtnCancel.TabIndex = 4;
            this.myCtlBtnCancel.Text = "Cancel";
            this.AutoScaleBaseSize = new Size(5, 13);
            base.ClientSize = new Size(0xfb, 0x84);
            base.Controls.Add(this.myCtlLblSwitchPage);
            base.Controls.Add(this.myCtlCmbItems);
            base.Controls.Add(this.myCtlChkSetSelectedPage);
            base.Controls.Add(this.myCtlBtnCancel);
            base.Controls.Add(this.myCtlBtnOK);
            base.FormBorderStyle = FormBorderStyle.FixedDialog;
            base.MaximizeBox = false;
            base.MinimizeBox = false;
            base.Name = "frmSwitchPages";
            base.ShowInTaskbar = false;
            this.Text = "Switch Pages";
            base.ResumeLayout(false);
        }

        protected override void OnLoad(EventArgs e)
        {
            foreach (MultiPanePage page in this.myDesigner.DesignedControl.Controls)
            {
                MultiPanePageItem item = new MultiPanePageItem(page);
                this.myCtlCmbItems.Items.Add(item);
                if (this.myDesigner.DesignerSelectedPage == page)
                {
                    this.myCtlCmbItems.SelectedItem = item;
                }
            }
        }

        public MultiPanePage FutureSelection
        {
            get
            {
                return this.myFutureSelectedItem;
            }
        }

        public bool SetSelectedPage
        {
            get
            {
                return this.mySetSelectedPage;
            }
        }

        private class MultiPanePageItem
        {
            private MultiPanePage myPage;

            public MultiPanePageItem(MultiPanePage thePg)
            {
                this.myPage = thePg;
            }

            public override string ToString()
            {
                return this.myPage.Name;
            }

            public MultiPanePage Page
            {
                get
                {
                    return this.myPage;
                }
            }
        }
    }
}

