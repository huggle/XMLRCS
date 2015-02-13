//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU Lesser General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU Lesser General Public License for more details.

// Copyright (c) Petr Bena benapetr@gmail.com

using System;
using System.Collections.Generic;
using System.Text;
using System.IO;
using System.Xml;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace XmlRcs
{
    public class EditEventArgs : EventArgs
    {
        public EditEventArgs(RecentChange change)
        {
            this.Change = change;
        }

        public RecentChange Change;
    }

    public class Provider
    {
        private StreamReader streamReader = null;
        private StreamWriter streamWriter = null;
        private NetworkStream networkStream = null;
        private TcpClient client = null;
        private DateTime lastPing;
        private string lineBuffer = "";
        private Byte[] buffer = new Byte[512];
        private List<string> lSubscriptions = null;
        private bool autoconn;

        public delegate void EditHandler(object sender, EditEventArgs args);
        public event EditHandler On_Change;

        public List<string> Subscriptions
        {
            get
            {
                if (!this.IsConnected)
                    return new List<string>();

                // we copy a local list
                return new List<string>(this.lSubscriptions);
            }
        }

        public bool IsConnected
        {
            get
            {
                if (this.client == null)
                    return false;
                return (this.client.Client.Connected);
            }
        }

        /// <summary>
        /// Creates a new provider
        /// </summary>
        /// <param name="autoreconnect">if true the provider will automatically try to reconnect in case it wasn't connected</param>
        public Provider(bool autoreconnect = false)
        {
            this.autoconn = autoreconnect;
        }

        private bool isConnected()
        {
            if (!this.IsConnected)
            {
                if (!this.autoconn)
                    return false;
                return this.Connect();
            }
            return true;
        }

        private void __evt_Edit(RecentChange change)
        {
            if (this.On_Change != null)
            {
                // send a signal
                // everywhere
                this.On_Change(this, new EditEventArgs(change));
            }
        }

        private void send(string data)
        {
            if (!this.IsConnected)
                return;
            this.streamWriter.WriteLine(data);
        }

        private void OnReceive(IAsyncResult data)
        {
            int bytes = this.networkStream.EndRead(data);
            string text = System.Text.Encoding.UTF8.GetString(buffer, 0, bytes);
            if (!text.Contains("\n"))
            {
                this.lineBuffer += text;
            }
            else
            {
                List<string> parts = new List<string>(text.Split('\n'));
                while (parts.Count > 0)
                {
                    this.lineBuffer += parts[0];
                    if (parts.Count > 1)
                    {
                        this.processOutput(this.lineBuffer + "\n");
                        this.lineBuffer = "";
                    }
                    parts.RemoveAt(0);
                }
            }
            this.resetCallback();
        }

        private static int TryParseIS(string input)
        {
            int result;
            if (!int.TryParse(input, out result))
                result = 0;
            return result;
        }

        private void processOutput(string data)
        {
            // put the text into XML document
            XmlDocument document = new XmlDocument();
            document.LoadXml(data);
            switch (document.DocumentElement.Name)
            {
                case "ping":
                    this.send("pong");
                    break;
                case "fatal":
                    break;
                case "error":
                    break;
                case "ok":
                    break;
                case "edit":
                    {
                        RecentChange rc = new RecentChange();
                        foreach (XmlAttribute item in document.DocumentElement.Attributes)
                        {
                            switch (item.Name)
                            {
                                case "wiki":
                                    rc.Wiki = item.Value;
                                    break;
                                case "server_name":
                                    rc.ServerName = item.Value;
                                    break;
                                case "summary":
                                    rc.Summary = item.Value;
                                    break;
                                case "revid":
                                    rc.RevID = TryParseIS(item.Value);
                                    break;
                                case "oldid":
                                    rc.OldID = TryParseIS(item.Value);
                                    break;
                                case "title":
                                    rc.Title = item.Value;
                                    break;
                                case "namespace":
                                    rc.Namespace = TryParseIS(item.Value);
                                    break;
                                case "user":
                                    rc.User = item.Value;
                                    break;
                                case "bot":
                                    rc.Bot = bool.Parse(item.Value);
                                    break;
                                case "patrolled":
                                    rc.Patrolled = bool.Parse(item.Value);
                                    break;
                                case "minor":
                                    rc.Minor = bool.Parse(item.Value);
                                    break;
                                case "type":
                                    {
                                        switch (item.Value.ToLower())
                                        {
                                            case "new":
                                                rc.Type = RecentChange.ChangeType.New;
                                                break;
                                            case "log":
                                                rc.Type = RecentChange.ChangeType.Log;
                                                break;
                                            case "edit":
                                                rc.Type = RecentChange.ChangeType.Edit;
                                                break;
                                        }
                                    }
                                    break;
                                case "length_new":
                                    rc.LengthNew = TryParseIS(item.Value);
                                    break;
                                case "length_old":
                                    rc.LengthOld = TryParseIS(item.Value);
                                    break;
                                case "timestamp":
                                    rc.Timestamp = Configuration.UnixTimeStampToDateTime(double.Parse(item.Value));
                                    break;
                            }
                        }
                        rc.OriginalXml = data;
                        this.__evt_Edit(rc);
                    }
                    break;
            }
        }

        private void resetCallback()
        {
            if (!string.IsNullOrEmpty(this.lineBuffer) && this.lineBuffer.EndsWith("\n"))
            {
                this.processOutput(this.lineBuffer);
                this.lineBuffer = "";
            }
            AsyncCallback callback = new AsyncCallback(OnReceive);
            this.networkStream.BeginRead(buffer, 0, buffer.Length, callback, this.networkStream);
        }

        /// <summary>
        /// Connect to XmlRcs server, this function needs to be called before you can start subscribing to changes on wiki
        /// </summary>
        /// <returns>True on success</returns>
        public bool Connect()
        {
            if (this.IsConnected)
                return false;

            this.lSubscriptions = new List<string>();
            this.client = new TcpClient(Configuration.Server, Configuration.Port);
            this.networkStream = this.client.GetStream();
            this.streamReader = new StreamReader(this.networkStream, Encoding.UTF8);
            this.streamWriter = new StreamWriter(this.networkStream, Encoding.UTF8);
            this.streamWriter.AutoFlush = true;
            // there is some weird bug in .Net that put garbage to first packet that is sent out
            // this is a dummy line that will flush out that garbage
            this.send("pong");
            this.lastPing = DateTime.Now;
            this.resetCallback();
            return true;
        }

        public bool Subscribe(string wiki)
        {
            if (!this.isConnected())
                return false;
            if (!this.lSubscriptions.Contains(wiki))
                this.lSubscriptions.Add(wiki);
            this.send("S " + wiki);
            return true;
        }

        public bool Unsubscribe(string wiki)
        {
            if (!this.isConnected())
                return false;
            if (this.lSubscriptions.Contains(wiki))
                this.lSubscriptions.Remove(wiki);
            this.send("D " + wiki);
            return true;
        }

        /// <summary>
        /// Disconnect from server
        /// </summary>
        public void Disconnect()
        {
            if (!this.IsConnected)
                return;

            this.send("exit");
            this.client.Close();
            this.networkStream = null;
            this.streamReader = null;
            this.streamWriter = null;
            this.client = null;
        }

        public bool Reconnect()
        {
            this.Disconnect();
            return this.Connect();
        }
    }
}
