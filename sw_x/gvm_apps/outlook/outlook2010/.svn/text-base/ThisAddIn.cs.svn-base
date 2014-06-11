using System;
using System.Collections;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Xml.Linq;
using System.Windows.Forms;
using Outlook = Microsoft.Office.Interop.Outlook;
using Office = Microsoft.Office.Core;

namespace outlook2010
{
    public partial class ThisAddIn
    {
        bool mSyncContacts;
        Timer mTimer;
        BackgroundWorker mWorker;
        Outlook.Inspectors mInspectors;

        /// <summary>Gets the root contacts folder.</summary>
        private Outlook.Folder RootContactsFolder
        {
            get
            {
                // Use the MAPI namespace to get the root contacts folder.
                Outlook.NameSpace nameSpace = this.Application.GetNamespace("MAPI");
                return nameSpace.GetDefaultFolder(Outlook.OlDefaultFolders.olFolderContacts) as Outlook.Folder;
            }
        }

        private void ThisAddIn_Startup(object sender, System.EventArgs e)
        {
            mInspectors = this.Application.Inspectors;
            mInspectors.NewInspector += new Outlook.InspectorsEvents_NewInspectorEventHandler(OnNewInspector);

            // TODO: sync RootContactsFolder and PIM
            foreach (Outlook.ContactItem item in RootContactsFolder.Items)
            {
                this.InitContact(item);
            }

            mWorker = new BackgroundWorker();
            mWorker.DoWork += new DoWorkEventHandler(WorkerPoll);
            mWorker.RunWorkerCompleted += new RunWorkerCompletedEventHandler(OnWorkerComplete);

            mTimer = new Timer();
            mTimer.Interval = 10000; // 10 seconds
            mTimer.Tick += new EventHandler(OnTick);
            mTimer.Start();

            mSyncContacts = false;
        }

        private void ThisAddIn_Shutdown(object sender, System.EventArgs e)
        {
            mTimer.Stop();
        }

        private void WorkerPoll(object sender, DoWorkEventArgs e)
        {
            ArrayList list = (ArrayList)e.Argument;
            bool outlookSync = (bool)(list[0]);
            List<Outlook.ContactItem> contacts = (List<Outlook.ContactItem>)(list[1]);

            // TODO: sync with PIM

            e.Result = (object)contacts;
        }

        private void OnTick(object sender, EventArgs e)
        {
            if (mWorker.IsBusy == false)
            {
                List<Outlook.ContactItem> contacts = new List<Outlook.ContactItem>();
                foreach (Outlook.ContactItem item in RootContactsFolder.Items)
                {
                    contacts.Add(item);
                }

                ArrayList list = new ArrayList();
                list.Add(mSyncContacts);
                list.Add(contacts);

                mWorker.RunWorkerAsync(list);
                mSyncContacts = false;
            }
        }

        private void OnWorkerComplete(object sender, RunWorkerCompletedEventArgs e)
        {
            List<Outlook.ContactItem> contacts = (List<Outlook.ContactItem>)e.Result;

            // TODO: add contacts to Outlook folder
        }

        private void InitContact(Outlook.ContactItem contact)
        {
            contact.AfterWrite += new Outlook.ItemEvents_10_AfterWriteEventHandler(OnSaveContact);
            contact.BeforeDelete += new Outlook.ItemEvents_10_BeforeDeleteEventHandler(OnDeleteContact);
        }

        private void OnDeleteContact(object Item, ref bool Cancel)
        {
            mSyncContacts = true;
        }

        private void OnSaveContact()
        {
            mSyncContacts = true;
        }

        /// triggered for a new contact, task, etc
        private void OnNewInspector(Outlook.Inspector Inspector)
        {
            Outlook.ContactItem contact = Inspector.CurrentItem as Outlook.ContactItem;
            if (contact != null)
            {
                this.InitContact(contact);
                mSyncContacts = true;
            }
        }

        private void CreateContactExample(string first, string last)
        {
            try
            {
                Outlook.ContactItem contact = Application.CreateItem(Outlook.OlItemType.olContactItem) as Outlook.ContactItem;
                contact.FirstName = first;
                contact.LastName = last;
                contact.JobTitle = "Account Representative";
                contact.CompanyName = "Contoso Ltd.";
                contact.OfficeLocation = "36/2529";
                contact.BusinessTelephoneNumber = "4255551212 x432";
                contact.WebPage = "http://www.contoso.com";
                contact.BusinessAddressStreet = "1 Microsoft Way";
                contact.BusinessAddressCity = "Redmond";
                contact.BusinessAddressState = "WA";
                contact.BusinessAddressPostalCode = "98052";
                contact.BusinessAddressCountry = "United States of America";
                contact.Email1Address = "melissa@contoso.com";
                contact.Email1AddressType = "SMTP";
                contact.Email1DisplayName = "Melissa MacBeth (mellissa@contoso.com)";
                contact.Save();
            }
            catch
            {
                MessageBox.Show("The new contact was not saved.");
            }
        }

        #region VSTO generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InternalStartup()
        {
            this.Startup += new System.EventHandler(ThisAddIn_Startup);
            this.Shutdown += new System.EventHandler(ThisAddIn_Shutdown);
        }

        #endregion
    }
}
