namespace Chilano.Common
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing;
    using System.Runtime.InteropServices;
    using System.Windows.Forms;

    public class CListView : ListView
    {
        private ArrayList _embeddedControls;
        private const int LVM_FIRST = 0x1000;
        private const int LVM_GETCOLUMNORDERARRAY = 0x103b;
        private const int WM_PAINT = 15;

        public CListView()
        {
            base.FullRowSelect = true;
            base.GridLines = true;
            this._embeddedControls = new ArrayList();
        }

        private void _embeddedControl_Click(object sender, EventArgs e)
        {
            foreach (EmbeddedControl control in this._embeddedControls)
            {
                if (control.Control == ((Control) sender))
                {
                    base.SelectedItems.Clear();
                    control.Item.Selected = true;
                }
            }
        }

        public void AddEmbeddedControl(Control c, int col, int row)
        {
            this.AddEmbeddedControl(c, col, row, DockStyle.Fill);
        }

        public void AddEmbeddedControl(Control c, int col, int row, DockStyle dock)
        {
            EmbeddedControl control;
            if (c == null)
            {
                throw new ArgumentNullException();
            }
            if ((col >= base.Columns.Count) || (row >= base.Items.Count))
            {
                throw new ArgumentOutOfRangeException();
            }
            control.Control = c;
            control.Col = col;
            control.Row = row;
            control.Dock = dock;
            control.Item = base.Items[row];
            this._embeddedControls.Add(control);
            c.Click += new EventHandler(this._embeddedControl_Click);
            base.Controls.Add(c);
        }

        public void CheckAll()
        {
            for (int i = 0; i < base.Items.Count; i++)
            {
                base.Items[i].Checked = true;
            }
        }

        public void CheckInverse()
        {
            for (int i = 0; i < base.Items.Count; i++)
            {
                base.Items[i].Checked = !base.Items[i].Checked;
            }
        }

        public void CheckNone()
        {
            for (int i = 0; i < base.Items.Count; i++)
            {
                base.Items[i].Checked = false;
            }
        }

        protected int[] GetColumnOrder()
        {
            IntPtr lPar = Marshal.AllocHGlobal((int) (Marshal.SizeOf(typeof(int)) * base.Columns.Count));
            if (SendMessage(base.Handle, 0x103b, new IntPtr(base.Columns.Count), lPar).ToInt32() == 0)
            {
                Marshal.FreeHGlobal(lPar);
                return null;
            }
            int[] destination = new int[base.Columns.Count];
            Marshal.Copy(lPar, destination, 0, base.Columns.Count);
            Marshal.FreeHGlobal(lPar);
            return destination;
        }

        public Control GetEmbeddedControl(int col, int row)
        {
            foreach (EmbeddedControl control in this._embeddedControls)
            {
                if ((control.Row == row) && (control.Col == col))
                {
                    return control.Control;
                }
            }
            return null;
        }

        protected Rectangle GetSubItemBounds(ListViewItem Item, int SubItem)
        {
            Rectangle empty = Rectangle.Empty;
            if (Item == null)
            {
                throw new ArgumentNullException("Item");
            }
            int[] columnOrder = this.GetColumnOrder();
            if (columnOrder == null)
            {
                return empty;
            }
            if (SubItem >= columnOrder.Length)
            {
                throw new IndexOutOfRangeException("SubItem " + SubItem + " out of range");
            }
            Rectangle bounds = Item.GetBounds(ItemBoundsPortion.Entire);
            int left = bounds.Left;
            int index = 0;
            while (index < columnOrder.Length)
            {
                ColumnHeader header = base.Columns[columnOrder[index]];
                if (header.Index == SubItem)
                {
                    break;
                }
                left += header.Width;
                index++;
            }
            return new Rectangle(left, bounds.Top, base.Columns[columnOrder[index]].Width, bounds.Height);
        }

        public void Remove(RemoveType Type)
        {
            if (Type == RemoveType.Matching)
            {
                throw new ArgumentException("Must specify a column and type to match.");
            }
            this.Remove(Type, 0, "");
        }

        public void Remove(ListViewItem Item)
        {
            if (this._embeddedControls.Count >= Item.Index)
            {
                EmbeddedControl control = (EmbeddedControl) this._embeddedControls[Item.Index];
                this.RemoveEmbeddedControl(Item);
            }
            base.Items.Remove(Item);
            for (int i = 0; i < this._embeddedControls.Count; i++)
            {
                EmbeddedControl control2 = (EmbeddedControl) this._embeddedControls[i];
                control2.Row = base.Items.IndexOf(control2.Item);
                this._embeddedControls[i] = control2;
            }
        }

        public void Remove(RemoveType Type, int Col, string Match)
        {
            switch (Type)
            {
                case RemoveType.All:
                    foreach (ListViewItem item in base.Items)
                    {
                        base.Items.Remove(item);
                    }
                    break;

                case RemoveType.Selected:
                    foreach (ListViewItem item in base.SelectedItems)
                    {
                        base.Items.Remove(item);
                    }
                    break;

                case RemoveType.Matching:
                    foreach (ListViewItem item in base.Items)
                    {
                        if (item.SubItems[Col].Text == Match)
                        {
                            base.Items.Remove(item);
                        }
                    }
                    break;
            }
            for (int i = 0; i < this._embeddedControls.Count; i++)
            {
                EmbeddedControl control = (EmbeddedControl) this._embeddedControls[i];
                control.Row = base.Items.IndexOf(control.Item);
                this._embeddedControls[i] = control;
            }
        }

        public void RemoveEmbeddedControl(Control c)
        {
            if (c != null)
            {
                for (int i = 0; i < this._embeddedControls.Count; i++)
                {
                    EmbeddedControl control = (EmbeddedControl) this._embeddedControls[i];
                    if (control.Control == c)
                    {
                        c.Click -= new EventHandler(this._embeddedControl_Click);
                        base.Controls.Remove(c);
                        this._embeddedControls.RemoveAt(i);
                        return;
                    }
                }
                throw new Exception("Control not found!");
            }
        }

        public void RemoveEmbeddedControl(ListViewItem l)
        {
            for (int i = 0; i < this._embeddedControls.Count; i++)
            {
                if (((EmbeddedControl) this._embeddedControls[i]).Item == l)
                {
                    this.RemoveEmbeddedControl(((EmbeddedControl) this._embeddedControls[i]).Control);
                }
            }
        }

        public void SelectAll()
        {
            for (int i = 0; i < base.Items.Count; i++)
            {
                base.Items[i].Selected = true;
            }
        }

        public void SelectInverse()
        {
            for (int i = 0; i < base.Items.Count; i++)
            {
                base.Items[i].Selected = !base.Items[i].Selected;
            }
        }

        public void SelectNone()
        {
            for (int i = 0; i < base.Items.Count; i++)
            {
                base.Items[i].Selected = false;
            }
        }

        public void SelectSimilar(int column)
        {
            if (base.SelectedItems.Count > 0)
            {
                string text = base.SelectedItems[0].SubItems[column].Text;
                for (int i = 0; i < base.Items.Count; i++)
                {
                    if (base.Items[i].SubItems[column].Text == text)
                    {
                        base.Items[i].Selected = true;
                    }
                }
            }
        }

        [DllImport("user32.dll")]
        private static extern IntPtr SendMessage(IntPtr hWnd, int msg, IntPtr wPar, IntPtr lPar);
        protected override void WndProc(ref Message m)
        {
            if ((m.Msg == 15) && (this.View == System.Windows.Forms.View.Details))
            {
                foreach (EmbeddedControl control in this._embeddedControls)
                {
                    Rectangle subItemBounds = this.GetSubItemBounds(control.Item, control.Col);
                    if ((base.HeaderStyle != ColumnHeaderStyle.None) && (subItemBounds.Top < this.Font.Height))
                    {
                        control.Control.Visible = false;
                        continue;
                    }
                    control.Control.Visible = true;
                    switch (control.Dock)
                    {
                        case DockStyle.None:
                            subItemBounds.Size = control.Control.Size;
                            break;

                        case DockStyle.Top:
                            subItemBounds.Height = control.Control.Height;
                            break;

                        case DockStyle.Bottom:
                            subItemBounds.Offset(0, subItemBounds.Height - control.Control.Height);
                            subItemBounds.Height = control.Control.Height;
                            break;

                        case DockStyle.Left:
                            subItemBounds.Width = control.Control.Width;
                            break;

                        case DockStyle.Right:
                            subItemBounds.Offset(subItemBounds.Width - control.Control.Width, 0);
                            subItemBounds.Width = control.Control.Width;
                            break;
                    }
                    control.Control.Bounds = subItemBounds;
                }
            }
            base.WndProc(ref m);
        }

        [DefaultValue(0)]
        public System.Windows.Forms.View View
        {
            get
            {
                return base.View;
            }
            set
            {
                foreach (EmbeddedControl control in this._embeddedControls)
                {
                    control.Control.Visible = value == System.Windows.Forms.View.Details;
                }
                base.View = value;
            }
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct EmbeddedControl
        {
            public System.Windows.Forms.Control Control;
            public ListViewItem Item;
            public DockStyle Dock;
            public int Row;
            public int Col;
            public EmbeddedControl(System.Windows.Forms.Control Control, ListViewItem Item, DockStyle Dock, int Row, int Col)
            {
                this.Control = Control;
                this.Item = Item;
                this.Dock = Dock;
                this.Row = Row;
                this.Col = Col;
            }
        }

        public enum RemoveType
        {
            All,
            Selected,
            Matching
        }
    }
}

