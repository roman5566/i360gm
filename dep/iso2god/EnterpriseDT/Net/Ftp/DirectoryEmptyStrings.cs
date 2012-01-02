namespace EnterpriseDT.Net.Ftp
{
    using System;

    public class DirectoryEmptyStrings : ServerStrings
    {
        public static string EMPTY_DIR = "EMPTY";
        public static string NO_DATA_SETS_FOUND = "NO DATA SETS FOUND";
        public static string NO_FILES = "NO FILES";
        public static string NO_SUCH_FILE_OR_DIR = "NO SUCH FILE OR DIRECTORY";

        public DirectoryEmptyStrings()
        {
            base.Add(NO_FILES);
            base.Add(NO_SUCH_FILE_OR_DIR);
            base.Add(EMPTY_DIR);
            base.Add(NO_DATA_SETS_FOUND);
        }
    }
}

