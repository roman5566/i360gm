namespace EnterpriseDT.Net.Ftp
{
    using System;
    using System.IO;
    using System.Runtime.CompilerServices;

    public interface IFileTransferClient
    {
        event BytesTransferredHandler BytesTransferred;

        event TransferHandler TransferCompleteEx;

        event TransferHandler TransferStartedEx;

        void CancelResume();
        void CancelTransfer();
        void CdUp();
        void ChDir(string dir);
        void Connect();
        void Delete(string remoteFile);
        string[] Dir();
        string[] Dir(string dirname);
        string[] Dir(string dirname, bool full);
        FTPFile[] DirDetails();
        FTPFile[] DirDetails(string dirname);
        bool Exists(string remoteFile);
        byte[] Get(string remoteFile);
        void Get(Stream destStream, string remoteFile);
        void Get(string localPath, string remoteFile);
        void MkDir(string dir);
        DateTime ModTime(string remoteFile);
        void Put(byte[] bytes, string remoteFile);
        long Put(Stream srcStream, string remoteFile);
        void Put(string localPath, string remoteFile);
        long Put(Stream srcStream, string remoteFile, bool append);
        void Put(byte[] bytes, string remoteFile, bool append);
        void Put(string localPath, string remoteFile, bool append);
        string Pwd();
        void Quit();
        void QuitImmediately();
        void Rename(string from, string to);
        void Resume();
        void RmDir(string dir);
        void SetModTime(string remoteFile, DateTime modTime);
        long Size(string remoteFile);

        bool CloseStreamsAfterTransfer { get; set; }

        int ControlPort { get; set; }

        bool DeleteOnFailure { get; set; }

        bool IsConnected { get; }

        string RemoteHost { get; set; }

        bool ShowHiddenFiles { get; set; }

        int Timeout { get; set; }

        int TransferBufferSize { get; set; }

        long TransferNotifyInterval { get; set; }

        bool TransferNotifyListings { get; set; }

        FTPTransferType TransferType { get; set; }
    }
}

