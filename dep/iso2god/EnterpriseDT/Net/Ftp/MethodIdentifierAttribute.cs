namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class MethodIdentifierAttribute : Attribute
    {
        private Type[] argumentTypes;
        private MethodIdentifier identifier;

        public MethodIdentifierAttribute(MethodIdentifier identifier)
        {
            this.argumentTypes = null;
            this.identifier = identifier;
            this.argumentTypes = new Type[0];
        }

        public MethodIdentifierAttribute(MethodIdentifier identifier, Type arg1) : this(identifier, arg1, null, null, null)
        {
            this.identifier = identifier;
            this.argumentTypes = new Type[] { arg1 };
        }

        public MethodIdentifierAttribute(MethodIdentifier identifier, Type arg1, Type arg2) : this(identifier, arg1, arg2, null, null)
        {
            this.identifier = identifier;
            this.argumentTypes = new Type[] { arg1, arg2 };
        }

        public MethodIdentifierAttribute(MethodIdentifier identifier, Type arg1, Type arg2, Type arg3) : this(identifier, arg1, arg2, arg3, null)
        {
            this.identifier = identifier;
            this.argumentTypes = new Type[] { arg1, arg2, arg3 };
        }

        public MethodIdentifierAttribute(MethodIdentifier identifier, Type arg1, Type arg2, Type arg3, Type arg4)
        {
            this.argumentTypes = null;
            this.identifier = identifier;
            this.argumentTypes = new Type[] { arg1, arg2, arg3, arg4 };
        }

        public Type[] ArgumentTypes
        {
            get
            {
                return this.argumentTypes;
            }
        }

        public MethodIdentifier Identifier
        {
            get
            {
                return this.identifier;
            }
        }
    }
}

