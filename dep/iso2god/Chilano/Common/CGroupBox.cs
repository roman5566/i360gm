namespace Chilano.Common
{
    using System;
    using System.Drawing;
    using System.Windows.Forms;

    public class CGroupBox : GroupBox
    {
        private Color borderColor = Color.Black;

        protected override void OnPaint(PaintEventArgs e)
        {
            Size size = TextRenderer.MeasureText(this.Text, this.Font);
            Rectangle clipRectangle = e.ClipRectangle;
            clipRectangle.Y += size.Height / 2;
            clipRectangle.Height -= size.Height / 2;
            ControlPaint.DrawBorder(e.Graphics, clipRectangle, this.borderColor, ButtonBorderStyle.Solid);
            Rectangle rect = e.ClipRectangle;
            rect.X += 6;
            rect.Width = size.Width;
            rect.Height = size.Height;
            e.Graphics.FillRectangle(new SolidBrush(this.BackColor), rect);
            e.Graphics.DrawString(this.Text, this.Font, new SolidBrush(this.ForeColor), rect);
        }

        public Color BorderColor
        {
            get
            {
                return this.borderColor;
            }
            set
            {
                this.borderColor = value;
            }
        }
    }
}

