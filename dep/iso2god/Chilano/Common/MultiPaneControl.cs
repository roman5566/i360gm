namespace Chilano.Common
{
    using Chilano.Common.Design;
    using System;
    using System.ComponentModel;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Runtime.CompilerServices;
    using System.Windows.Forms;

    [Designer(typeof(MultiPaneControlDesigner)), ToolboxItem(typeof(MultiPaneControlToolboxItem))]
    public class MultiPaneControl : Control
    {
        protected MultiPanePage mySelectedPage;
        protected static readonly Size ourDefaultSize = new Size(200, 100);

        public event EventHandler SelectedPageChanged;

        public event EventHandler SelectedPageChanging;

        public MultiPaneControl()
        {
            base.ControlAdded += new ControlEventHandler(this.Handler_ControlAdded);
            base.SizeChanged += new EventHandler(this.Handler_SizeChanged);
            base.SetStyle(ControlStyles.SupportsTransparentBackColor, true);
            this.BackColor = Color.Transparent;
        }

        private void Handler_ControlAdded(object theSender, ControlEventArgs theArgs)
        {
            if (theArgs.Control is MultiPanePage)
            {
                MultiPanePage control = (MultiPanePage) theArgs.Control;
                control.Location = new Point(0, 0);
                control.Size = base.ClientSize;
                control.Dock = DockStyle.Fill;
                if (this.SelectedPage == null)
                {
                    this.SelectedPage = control;
                }
                else
                {
                    control.Visible = false;
                }
            }
            else
            {
                base.Controls.Remove(theArgs.Control);
            }
        }

        private void Handler_SizeChanged(object sender, EventArgs e)
        {
            foreach (MultiPanePage page in base.Controls)
            {
                page.Size = base.ClientSize;
            }
        }

        protected override Size DefaultSize
        {
            get
            {
                return ourDefaultSize;
            }
        }

        [Editor(typeof(MultiPaneControlSelectedPageEditor), typeof(UITypeEditor))]
        public MultiPanePage SelectedPage
        {
            get
            {
                return this.mySelectedPage;
            }
            set
            {
                if (this.mySelectedPage != value)
                {
                    if (this.SelectedPageChanging != null)
                    {
                        this.SelectedPageChanging(this, EventArgs.Empty);
                    }
                    if (this.mySelectedPage != null)
                    {
                        this.mySelectedPage.Visible = false;
                    }
                    this.mySelectedPage = value;
                    this.mySelectedPage.Refresh();
                    if (this.mySelectedPage != null)
                    {
                        this.mySelectedPage.Visible = true;
                    }
                    if (this.SelectedPageChanged != null)
                    {
                        this.SelectedPageChanged(this, EventArgs.Empty);
                    }
                }
            }
        }
    }
}

