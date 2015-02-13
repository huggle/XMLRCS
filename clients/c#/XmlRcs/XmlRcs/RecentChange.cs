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

namespace XmlRcs
{
    /// <summary>
    /// Recent change item as provided by XmlRcs, if any string was not provided, it will be null
    /// You can also run EmptyNulls() in order to initialize all null strings with empty string
    /// </summary>
    public class RecentChange
    {
        public enum ChangeType
        {
            Edit,
            Log,
            New,
            Unknown
        }

        public void EmptyNulls()
        {
            if (this.Wiki == null)
                this.Wiki = "";
            if (this.ServerName == null)
                this.ServerName = "";
            if (this.Title == null)
                this.Title = "";
            if (this.User == null)
                this.User = "";
            if (this.Summary == null)
                this.Summary = "";
        }
        /// <summary>
        /// Internal name of wiki
        /// </summary>
        public string Wiki = null;
        public string ServerName = null;
        public string Title = null;
        public int Namespace = 0;
        public int RevID = 0;
        public int OldID = 0;
        public string User = null;
        public bool Bot = false;
        public bool Patrolled = false;
        public bool Minor = false;
        public ChangeType Type = ChangeType.Unknown;
        public int LengthNew = 0;
        public int LengthOld = 0;
        public string Summary = null;
        public string OriginalXml;
        /// <summary>
        /// If no timestamp was provided this will equal minimal time
        /// </summary>
        public DateTime Timestamp = DateTime.MinValue;
    }
}
