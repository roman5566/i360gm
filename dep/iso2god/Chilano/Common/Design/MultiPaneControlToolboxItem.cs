namespace Chilano.Common.Design
{
    using Chilano.Common;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing.Design;
    using System.Runtime.Serialization;
    using System.Security.Permissions;

    [Serializable, PermissionSet(SecurityAction.Demand, Name="FullTrust")]
    public class MultiPaneControlToolboxItem : ToolboxItem
    {
        public MultiPaneControlToolboxItem() : base(typeof(MultiPaneControl))
        {
        }

        public MultiPaneControlToolboxItem(SerializationInfo theInfo, StreamingContext theContext)
        {
            this.Deserialize(theInfo, theContext);
        }

        protected override IComponent[] CreateComponentsCore(IDesignerHost theHost)
        {
            return (DesignerTransactionUtility.DoInTransaction(theHost, "MultiPaneControlTooblxItem_CreateControl", new TransactionAwareParammedMethod(this.CreateControlWithOnePage), null) as IComponent[]);
        }

        public object CreateControlWithOnePage(IDesignerHost theHost, object theParam)
        {
            MultiPaneControl control = (MultiPaneControl) theHost.CreateComponent(typeof(MultiPaneControl));
            MultiPanePage page = (MultiPanePage) theHost.CreateComponent(typeof(MultiPanePage));
            control.Controls.Add(page);
            return new IComponent[] { control };
        }
    }
}

