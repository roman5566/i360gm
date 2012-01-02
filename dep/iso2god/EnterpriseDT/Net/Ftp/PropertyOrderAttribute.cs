namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class PropertyOrderAttribute : Attribute
    {
        private int order;

        public PropertyOrderAttribute(int order)
        {
            this.order = order;
        }

        public int Order
        {
            get
            {
                return this.order;
            }
            set
            {
                this.order = value;
            }
        }
    }
}

