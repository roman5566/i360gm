namespace Chilano.Common
{
    using System;
    using System.Windows.Forms;

    public class CTextLog : RichTextBox
    {
        public CTextLog()
        {
            this.Multiline = true;
            base.ScrollBars = RichTextBoxScrollBars.ForcedVertical;
        }

        public void Add(string LogMessage)
        {
            this.Text = this.Text + LogMessage + "\n";
            base.ScrollToCaret();
        }
    }
}

