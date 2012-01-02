namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.ComponentModel;

    internal class FTPComponentLinker
    {
        public static Component Find(ISite site, Type componentType)
        {
            if ((site != null) && (site.Container != null))
            {
                foreach (object obj2 in site.Container.Components)
                {
                    if (componentType.IsInstanceOfType(obj2))
                    {
                        return (Component) obj2;
                    }
                }
            }
            return null;
        }

        public static void Link(ISite site, IFTPComponent component)
        {
            if (((site != null) && site.DesignMode) && (site.Container != null))
            {
                foreach (object obj2 in site.Container.Components)
                {
                    if ((obj2 != component) && (obj2 is IFTPComponent))
                    {
                        ((IFTPComponent) obj2).LinkComponent(component);
                    }
                }
            }
        }
    }
}

