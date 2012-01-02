namespace Chilano.Common.Design
{
    using Chilano.Common;
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Security.Permissions;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    [PermissionSet(SecurityAction.Demand, Name="FullTrust")]
    public class MultiPaneControlDesigner : ParentControlDesigner
    {
        private DesignerVerb myAddVerb;
        private bool myInTransaction = false;
        private DesignerVerb myRemoveVerb;
        private MultiPanePage mySelectedPage;
        private DesignerVerb mySwitchVerb;
        private DesignerVerbCollection myVerbs;

        public override bool CanParent(Control theControl)
        {
            return ((theControl is MultiPanePage) && !this.Control.Contains(theControl));
        }

        private void CheckVerbStatus()
        {
            if (this.Control == null)
            {
                this.myRemoveVerb.Enabled = this.myAddVerb.Enabled = this.mySwitchVerb.Enabled = false;
            }
            else
            {
                this.myAddVerb.Enabled = true;
                this.myRemoveVerb.Enabled = this.Control.Controls.Count > 1;
                this.mySwitchVerb.Enabled = this.Control.Controls.Count > 1;
            }
        }

        protected override void Dispose(bool theDisposing)
        {
            if (theDisposing)
            {
                ISelectionService service = (ISelectionService) this.GetService(typeof(ISelectionService));
                if (service != null)
                {
                    service.SelectionChanged -= new EventHandler(this.Handler_SelectionChanged);
                }
                IComponentChangeService service2 = (IComponentChangeService) this.GetService(typeof(IComponentChangeService));
                if (service2 != null)
                {
                    service2.ComponentRemoving -= new ComponentEventHandler(this.Handler_ComponentRemoving);
                    service2.ComponentChanged -= new ComponentChangedEventHandler(this.Handler_ComponentChanged);
                }
                this.DesignedControl.SelectedPageChanged -= new EventHandler(this.Handler_SelectedPageChanged);
            }
            base.Dispose(theDisposing);
        }

        private static MultiPanePage GetPageOfControl(object theControl)
        {
            if (!(theControl is Control))
            {
                return null;
            }
            Control parent = (Control) theControl;
            while ((parent != null) && !(parent is MultiPanePage))
            {
                parent = parent.Parent;
            }
            return (MultiPanePage) parent;
        }

        private MultiPanePageDesigner GetSelectedPageDesigner()
        {
            MultiPanePage mySelectedPage = this.mySelectedPage;
            if (mySelectedPage == null)
            {
                return null;
            }
            MultiPanePageDesigner designer = null;
            IDesignerHost service = (IDesignerHost) this.GetService(typeof(IDesignerHost));
            if (service != null)
            {
                designer = (MultiPanePageDesigner) service.GetDesigner(mySelectedPage);
            }
            return designer;
        }

        private void Handler_AddPage(object theSender, EventArgs theArgs)
        {
            IDesignerHost service = (IDesignerHost) this.GetService(typeof(IDesignerHost));
            if (service != null)
            {
                this.myInTransaction = true;
                DesignerTransactionUtility.DoInTransaction(service, "MultiPaneControlAddPage", new TransactionAwareParammedMethod(this.Transaction_AddPage), null);
                this.myInTransaction = false;
            }
        }

        private void Handler_ComponentChanged(object theSender, ComponentChangedEventArgs theArgs)
        {
            this.CheckVerbStatus();
        }

        private void Handler_ComponentRemoving(object theSender, ComponentEventArgs theArgs)
        {
            if (theArgs.Component is MultiPanePage)
            {
                MultiPaneControl designedControl = this.DesignedControl;
                MultiPanePage component = (MultiPanePage) theArgs.Component;
                if (designedControl.Controls.Contains(component))
                {
                    IDesignerHost service = (IDesignerHost) this.GetService(typeof(IDesignerHost));
                    if (!this.myInTransaction)
                    {
                        this.myInTransaction = true;
                        DesignerTransactionUtility.DoInTransaction(service, "MultiPaneControlRemoveComponent", new TransactionAwareParammedMethod(this.Transaction_UpdateSelectedPage), null);
                        this.myInTransaction = false;
                    }
                    else
                    {
                        this.Transaction_UpdateSelectedPage(service, null);
                    }
                    this.CheckVerbStatus();
                }
            }
        }

        private void Handler_RemovePage(object sender, EventArgs eevent)
        {
            MultiPaneControl designedControl = this.DesignedControl;
            if ((designedControl != null) && (designedControl.Controls.Count >= 1))
            {
                IDesignerHost service = (IDesignerHost) this.GetService(typeof(IDesignerHost));
                if (service != null)
                {
                    this.myInTransaction = true;
                    DesignerTransactionUtility.DoInTransaction(service, "MultiPaneControlRemovePage", new TransactionAwareParammedMethod(this.Transaction_RemovePage), null);
                    this.myInTransaction = false;
                }
            }
        }

        private void Handler_SelectedPageChanged(object theSender, EventArgs theArgs)
        {
            this.mySelectedPage = this.DesignedControl.SelectedPage;
        }

        private void Handler_SelectionChanged(object sender, EventArgs e)
        {
            ISelectionService service = (ISelectionService) this.GetService(typeof(ISelectionService));
            if (service != null)
            {
                ICollection selectedComponents = service.GetSelectedComponents();
                MultiPaneControl designedControl = this.DesignedControl;
                foreach (object obj2 in selectedComponents)
                {
                    MultiPanePage pageOfControl = GetPageOfControl(obj2);
                    if ((pageOfControl != null) && (pageOfControl.Parent == designedControl))
                    {
                        this.DesignerSelectedPage = pageOfControl;
                        break;
                    }
                }
            }
        }

        private void Handler_SwitchPage(object theSender, EventArgs theArgs)
        {
            frmSwitchPages pages = new frmSwitchPages(this);
            if (pages.ShowDialog() == DialogResult.OK)
            {
                if (pages.SetSelectedPage)
                {
                    IDesignerHost service = (IDesignerHost) this.GetService(typeof(IDesignerHost));
                    if (service != null)
                    {
                        DesignerTransactionUtility.DoInTransaction(service, "MultiPaneControlSetSelectedPageAsConcrete", new TransactionAwareParammedMethod(this.Transaction_SetSelectedPageAsConcrete), pages.FutureSelection);
                    }
                }
                else
                {
                    this.DesignerSelectedPage = pages.FutureSelection;
                }
            }
        }

        public override void Initialize(IComponent theComponent)
        {
            base.Initialize(theComponent);
            ISelectionService service = (ISelectionService) this.GetService(typeof(ISelectionService));
            if (service != null)
            {
                service.SelectionChanged += new EventHandler(this.Handler_SelectionChanged);
            }
            IComponentChangeService service2 = (IComponentChangeService) this.GetService(typeof(IComponentChangeService));
            if (service2 != null)
            {
                service2.ComponentRemoving += new ComponentEventHandler(this.Handler_ComponentRemoving);
                service2.ComponentChanged += new ComponentChangedEventHandler(this.Handler_ComponentChanged);
            }
            this.DesignedControl.SelectedPageChanged += new EventHandler(this.Handler_SelectedPageChanged);
            this.myAddVerb = new DesignerVerb("Add page", new EventHandler(this.Handler_AddPage));
            this.myRemoveVerb = new DesignerVerb("Remove page", new EventHandler(this.Handler_RemovePage));
            this.mySwitchVerb = new DesignerVerb("Switch pages...", new EventHandler(this.Handler_SwitchPage));
            this.myVerbs = new DesignerVerbCollection();
            this.myVerbs.AddRange(new DesignerVerb[] { this.myAddVerb, this.myRemoveVerb, this.mySwitchVerb });
        }

        protected override void OnDragDrop(DragEventArgs theDragEvents)
        {
            MultiPanePageDesigner selectedPageDesigner = this.GetSelectedPageDesigner();
            if (selectedPageDesigner != null)
            {
                selectedPageDesigner.InternalOnDragDrop(theDragEvents);
            }
        }

        protected override void OnDragEnter(DragEventArgs theDragEvents)
        {
            MultiPanePageDesigner selectedPageDesigner = this.GetSelectedPageDesigner();
            if (selectedPageDesigner != null)
            {
                selectedPageDesigner.InternalOnDragEnter(theDragEvents);
            }
        }

        protected override void OnDragLeave(EventArgs theArgs)
        {
            MultiPanePageDesigner selectedPageDesigner = this.GetSelectedPageDesigner();
            if (selectedPageDesigner != null)
            {
                selectedPageDesigner.InternalOnDragLeave(theArgs);
            }
        }

        protected override void OnDragOver(DragEventArgs theDragEvents)
        {
            MultiPaneControl designedControl = this.DesignedControl;
            Point pt = designedControl.PointToClient(new Point(theDragEvents.X, theDragEvents.Y));
            if (!designedControl.DisplayRectangle.Contains(pt))
            {
                theDragEvents.Effect = DragDropEffects.None;
            }
            else
            {
                MultiPanePageDesigner selectedPageDesigner = this.GetSelectedPageDesigner();
                if (selectedPageDesigner != null)
                {
                    selectedPageDesigner.InternalOnDragOver(theDragEvents);
                }
            }
        }

        protected override void OnGiveFeedback(GiveFeedbackEventArgs e)
        {
            MultiPanePageDesigner selectedPageDesigner = this.GetSelectedPageDesigner();
            if (selectedPageDesigner != null)
            {
                selectedPageDesigner.InternalOnGiveFeedback(e);
            }
        }

        private object Transaction_AddPage(IDesignerHost theHost, object theParam)
        {
            MultiPaneControl designedControl = this.DesignedControl;
            MultiPanePage page = (MultiPanePage) theHost.CreateComponent(typeof(MultiPanePage));
            MemberDescriptor member = TypeDescriptor.GetProperties(designedControl)["Controls"];
            base.RaiseComponentChanging(member);
            designedControl.Controls.Add(page);
            this.DesignerSelectedPage = page;
            base.RaiseComponentChanged(member, null, null);
            return null;
        }

        private object Transaction_RemovePage(IDesignerHost theHost, object theParam)
        {
            if (this.mySelectedPage != null)
            {
                MemberDescriptor member = TypeDescriptor.GetProperties(this.DesignedControl)["Controls"];
                base.RaiseComponentChanging(member);
                try
                {
                    theHost.DestroyComponent(this.mySelectedPage);
                }
                catch
                {
                }
                base.RaiseComponentChanged(member, null, null);
            }
            return null;
        }

        private object Transaction_SetSelectedPageAsConcrete(IDesignerHost theHost, object theParam)
        {
            MultiPaneControl designedControl = this.DesignedControl;
            MemberDescriptor member = TypeDescriptor.GetProperties(designedControl)["SelectedPage"];
            base.RaiseComponentChanging(member);
            designedControl.SelectedPage = (MultiPanePage) theParam;
            base.RaiseComponentChanged(member, null, null);
            return null;
        }

        private object Transaction_UpdateSelectedPage(IDesignerHost theHost, object theParam)
        {
            MultiPaneControl designedControl = this.DesignedControl;
            MultiPanePage mySelectedPage = this.mySelectedPage;
            int index = designedControl.Controls.IndexOf(this.mySelectedPage);
            if (this.mySelectedPage == designedControl.SelectedPage)
            {
                MemberDescriptor member = TypeDescriptor.GetProperties(designedControl)["SelectedPage"];
                base.RaiseComponentChanging(member);
                if (designedControl.Controls.Count > 1)
                {
                    if (index == (designedControl.Controls.Count - 1))
                    {
                        designedControl.SelectedPage = (MultiPanePage) designedControl.Controls[index - 1];
                    }
                    else
                    {
                        designedControl.SelectedPage = (MultiPanePage) designedControl.Controls[index + 1];
                    }
                }
                else
                {
                    designedControl.SelectedPage = null;
                }
                base.RaiseComponentChanged(member, null, null);
            }
            else if (designedControl.Controls.Count > 1)
            {
                if (index == (designedControl.Controls.Count - 1))
                {
                    this.DesignerSelectedPage = (MultiPanePage) designedControl.Controls[index - 1];
                }
                else
                {
                    this.DesignerSelectedPage = (MultiPanePage) designedControl.Controls[index + 1];
                }
            }
            else
            {
                this.DesignerSelectedPage = null;
            }
            return null;
        }

        public MultiPaneControl DesignedControl
        {
            get
            {
                return (MultiPaneControl) this.Control;
            }
        }

        public MultiPanePage DesignerSelectedPage
        {
            get
            {
                return this.mySelectedPage;
            }
            set
            {
                if (this.mySelectedPage != null)
                {
                    this.mySelectedPage.Visible = false;
                }
                this.mySelectedPage = value;
                if (this.mySelectedPage != null)
                {
                    this.mySelectedPage.Visible = true;
                }
            }
        }

        public override DesignerVerbCollection Verbs
        {
            get
            {
                return this.myVerbs;
            }
        }
    }
}

