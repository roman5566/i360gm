namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class FileNotFoundStrings : ServerStrings
    {
        public const string CANNOT_FIND_THE_FILE = "CANNOT FIND THE FILE";
        public const string COULD_NOT_GET_FILE = "COULD NOT GET FILE";
        public const string DOES_NOT_EXIST = "DOES NOT EXIST";
        public const string FAILED_TO_OPEN_FILE = "FAILED TO OPEN FILE";
        public static string FILE_NOT_FOUND = "NOT FOUND";
        public const string NO_SUCH_FILE = "NO SUCH FILE";

        public FileNotFoundStrings()
        {
            base.Add(FILE_NOT_FOUND);
            base.Add("NO SUCH FILE");
            base.Add("CANNOT FIND THE FILE");
            base.Add("FAILED TO OPEN FILE");
            base.Add("COULD NOT GET FILE");
            base.Add("DOES NOT EXIST");
        }
    }
}

