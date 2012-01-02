namespace Chilano.Common.Design
{
    using Chilano.Common;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    internal class MultiPaneControlSelectedPageEditor : ObjectSelectorEditor
    {
        protected override void FillTreeWithData(ObjectSelectorEditor.Selector theSel, ITypeDescriptorContext theCtx, IServiceProvider theProvider)
        {
            base.FillTreeWithData(theSel, theCtx, theProvider);
            MultiPaneControl instance = (MultiPaneControl) theCtx.Instance;
            foreach (MultiPanePage page in instance.Controls)
            {
                ObjectSelectorEditor.SelectorNode node = new ObjectSelectorEditor.SelectorNode(page.Name, page);
                theSel.Nodes.Add(node);
                if (page == instance.SelectedPage)
                {
                    theSel.SelectedNode = node;
                }
            }
        }
    }
}

