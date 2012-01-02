namespace Chilano.Common
{
    using System;
    using System.Drawing;
    using System.Windows.Forms;

    public class CButton : Button
    {
        private Bitmap bDown;
        private Bitmap bNormal;
        private Bitmap bOver;
        private bool isDown = false;
        private bool isOver = false;

        public CButton()
        {
            base.MouseEnter += new EventHandler(this.CButton_MouseEnter);
            base.MouseLeave += new EventHandler(this.CButton_MouseLeave);
            base.MouseDown += new MouseEventHandler(this.CButton_MouseDown);
            base.MouseUp += new MouseEventHandler(this.CButton_MouseUp);
        }

        private void CButton_MouseDown(object sender, MouseEventArgs e)
        {
            this.isDown = true;
            this.BackgroundImage = this.bDown;
        }

        private void CButton_MouseEnter(object sender, EventArgs e)
        {
            this.isOver = true;
            this.BackgroundImage = this.bOver;
        }

        private void CButton_MouseLeave(object sender, EventArgs e)
        {
            this.isOver = false;
            this.BackgroundImage = this.bNormal;
        }

        private void CButton_MouseUp(object sender, MouseEventArgs e)
        {
            this.isDown = false;
            this.BackgroundImage = this.isOver ? this.bOver : this.bNormal;
        }

        public void SetImages(Bitmap Normal, Bitmap Over, Bitmap Down)
        {
            this.bNormal = Normal;
            this.bOver = Over;
            this.bDown = Down;
            this.BackgroundImage = Normal;
        }
    }
}

