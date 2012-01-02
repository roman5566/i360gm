namespace Chilano.Common.Design
{
    using Chilano.Common;
    using System;
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    internal class MultiPanePageDesigner : ScrollableControlDesigner
    {
        private Pen myBorderPen_Dark;
        private Pen myBorderPen_Light;
        private bool myMouseMovement = false;
        private int myOrigX = -1;
        private int myOrigY = -1;

        public override bool CanBeParentedTo(IDesigner theParentDesigner)
        {
            return ((theParentDesigner != null) && (theParentDesigner.Component is MultiPaneControl));
        }

        protected override void Dispose(bool theDisposing)
        {
            if (theDisposing)
            {
                if (this.myBorderPen_Dark != null)
                {
                    this.myBorderPen_Dark.Dispose();
                }
                if (this.myBorderPen_Light != null)
                {
                    this.myBorderPen_Light.Dispose();
                }
            }
            base.Dispose(theDisposing);
        }

        protected void DrawBorder(Graphics theG)
        {
            MultiPanePage designedControl = this.DesignedControl;
            if ((designedControl != null) && designedControl.Visible)
            {
                Rectangle clientRectangle = designedControl.ClientRectangle;
                clientRectangle.Width--;
                clientRectangle.Height--;
                theG.DrawRectangle(this.BorderPen, clientRectangle);
            }
        }

        protected override bool GetHitTest(Point pt)
        {
            return false;
        }

        private MultiPaneControlDesigner GetParentControlDesigner()
        {
            MultiPaneControlDesigner designer = null;
            if ((this.Control != null) && (this.Control.Parent != null))
            {
                IDesignerHost service = (IDesignerHost) this.GetService(typeof(IDesignerHost));
                if (service != null)
                {
                    designer = (MultiPaneControlDesigner) service.GetDesigner(this.Control.Parent);
                }
            }
            return designer;
        }

        private static Pen InternalCreatePen(Color theClr)
        {
            return new Pen(theClr) { DashStyle = DashStyle.Dash };
        }

        private Pen InternalEnsureDarkPenCreated()
        {
            if (this.myBorderPen_Dark == null)
            {
                this.myBorderPen_Dark = InternalCreatePen(Color.Black);
            }
            return this.myBorderPen_Dark;
        }

        private Pen InternalEnsureLightPenCreated()
        {
            if (this.myBorderPen_Light == null)
            {
                this.myBorderPen_Light = InternalCreatePen(Color.White);
            }
            return this.myBorderPen_Light;
        }

        internal void InternalOnDragDrop(DragEventArgs theArgs)
        {
            this.OnDragDrop(theArgs);
        }

        internal void InternalOnDragEnter(DragEventArgs theArgs)
        {
            this.OnDragEnter(theArgs);
        }

        internal void InternalOnDragLeave(EventArgs theArgs)
        {
            this.OnDragLeave(theArgs);
        }

        internal void InternalOnDragOver(DragEventArgs theArgs)
        {
            this.OnDragOver(theArgs);
        }

        internal void InternalOnGiveFeedback(GiveFeedbackEventArgs theArgs)
        {
            this.OnGiveFeedback(theArgs);
        }

        protected override void OnMouseDragBegin(int theX, int theY)
        {
            this.myOrigX = theX;
            this.myOrigY = theY;
        }

        protected override void OnMouseDragEnd(bool theCancel)
        {
            bool flag = !this.myMouseMovement && (this.Control.Parent != null);
            if (flag)
            {
                ISelectionService service = (ISelectionService) this.GetService(typeof(ISelectionService));
                if (service != null)
                {
                    service.SetSelectedComponents(new Control[] { this.Control.Parent });
                }
                else
                {
                    flag = false;
                }
            }
            if (!flag)
            {
                base.OnMouseDragEnd(theCancel);
            }
            this.myMouseMovement = false;
        }

        protected override void OnMouseDragMove(int theX, int theY)
        {
            if ((((theX > (this.myOrigX + 3)) || (theX < (this.myOrigX - 3))) || (theY > (this.myOrigY + 3))) || (theY < (this.myOrigY - 3)))
            {
                this.myMouseMovement = true;
                base.OnMouseDragBegin(this.myOrigX, this.myOrigY);
                base.OnMouseDragMove(theX, theY);
            }
        }

        protected override void OnPaintAdornments(PaintEventArgs pe)
        {
            this.DrawBorder(pe.Graphics);
            base.OnPaintAdornments(pe);
        }

        protected Pen BorderPen
        {
            get
            {
                if (this.Control.BackColor.GetBrightness() < 0.5)
                {
                    return this.InternalEnsureLightPenCreated();
                }
                return this.InternalEnsureDarkPenCreated();
            }
        }

        protected MultiPanePage DesignedControl
        {
            get
            {
                return (MultiPanePage) this.Control;
            }
        }

        public override System.Windows.Forms.Design.SelectionRules SelectionRules
        {
            get
            {
                System.Windows.Forms.Design.SelectionRules selectionRules = base.SelectionRules;
                if (this.Control.Parent is MultiPaneControl)
                {
                    selectionRules &= ~System.Windows.Forms.Design.SelectionRules.AllSizeable;
                }
                return selectionRules;
            }
        }

        public override DesignerVerbCollection Verbs
        {
            get
            {
                DesignerVerbCollection verbs = new DesignerVerbCollection();
                foreach (DesignerVerb verb in base.Verbs)
                {
                    verbs.Add(verb);
                }
                MultiPaneControlDesigner parentControlDesigner = this.GetParentControlDesigner();
                if (parentControlDesigner != null)
                {
                    foreach (DesignerVerb verb in parentControlDesigner.Verbs)
                    {
                        verbs.Add(verb);
                    }
                }
                return verbs;
            }
        }
    }
}

