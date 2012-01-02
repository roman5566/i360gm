namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.Reflection;

    public class ServerStrings : CollectionBase
    {
        private PropertyChangedEventHandler propertyChangeHandler;

        public void Add(string str)
        {
            base.List.Add(str);
            this.OnMemberChanged();
        }

        public void AddRange(string[] strs)
        {
            foreach (string str in strs)
            {
                base.List.Add(str);
            }
            this.OnMemberChanged();
        }

        public bool Contains(string str)
        {
            return base.List.Contains(str);
        }

        public void CopyTo(string[] array, int index)
        {
            base.List.CopyTo(array, index);
        }

        public int IndexOf(string str)
        {
            return base.List.IndexOf(str);
        }

        public void Insert(int index, string str)
        {
            base.List.Insert(index, str);
            this.OnMemberChanged();
        }

        public bool Matches(string reply)
        {
            string str = reply.ToUpper();
            for (int i = 0; i < base.Count; i++)
            {
                string str2 = this[i];
                if (str.IndexOf(str2.ToUpper()) >= 0)
                {
                    return true;
                }
            }
            return false;
        }

        private void OnMemberChanged()
        {
            if (this.propertyChangeHandler != null)
            {
                this.propertyChangeHandler(this, new PropertyChangedEventArgs(null));
            }
        }

        public void Remove(string str)
        {
            base.List.Remove(str);
            this.OnMemberChanged();
        }

        public string this[int index]
        {
            get
            {
                return (string) base.List[index];
            }
            set
            {
                if (((string) base.List[index]) != value)
                {
                    base.List[index] = value;
                    this.OnMemberChanged();
                }
            }
        }

        internal PropertyChangedEventHandler PropertyChangeHandler
        {
            get
            {
                return this.propertyChangeHandler;
            }
            set
            {
                this.propertyChangeHandler = value;
            }
        }
    }
}

