/*
  KeeFox - Allows Firefox to communicate with KeePass (via the KeeICE KeePass-plugin)
  Copyright 2008-2009 Chris Tomlinson <keefox@christomlinson.name>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// This is the implemenation of the XPCOM methods described in comp.idl

#include "comp-impl.h"

#if _MSC_VER
#pragma warning( push, 1 )
#pragma warning( disable : 4512 )
#pragma warning( disable : 4100 )
#pragma warning( disable : 4702 )
#endif

#include "nsICategoryManager.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMArray.h"
#include "nsIObserverService.h"
#include "nsServiceManagerUtils.h"
#include "nsILocalFile.h"
#include "nsIProxyObjectManager.h"
#include "nsISimpleEnumerator.h"
#include "nsIPrefService.h"

#include "shellapi.h"
#include "shlobj.h"

#include <string>
#include <vector>
#include <iostream>

#define IMPLEMENT_VISTA_TOOLS
#include "VistaTools.cxx"

#if _MSC_VER
#pragma warning( pop ) 
#endif

using namespace std;
using namespace KeeICE::KPlib;

using std::string;
using std::vector;

//NS_IMPL_ISUPPORTS1(CKeePass, IKeePass)
NS_IMPL_ADDREF(CKeeFox)
NS_IMPL_RELEASE(CKeeFox)

NS_INTERFACE_MAP_BEGIN(CKeeFox)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, CKeeFox)
  NS_INTERFACE_MAP_ENTRY(IKeeFox)
NS_INTERFACE_MAP_END

KeeFoxObserver *KeeFoxCallBackHelper::_observer = NULL;
bool KeeFoxCallBackHelper::javascriptCallBacksReady = false;


class CallbackReceiverI : public CallbackReceiver
{
public:

    virtual void
    callback(Ice::Int num, const Ice::Current&)
    {
		//cout << "received callback ###" << num << endl;
		
		// send a synchronous callback to the javascript observer. We don't
		// really care if the observer has gone since there's nothing we can
		// do about that.
		if (KeeFoxCallBackHelper::javascriptCallBacksReady)
			KeeFoxCallBackHelper::_observer->CallBackToKeeFoxJS(num);

		// we want this to be the last callback made in some situations (i.e. KeePass closing)
		if (num == 12)
		{
			KeeFoxCallBackHelper::javascriptCallBacksReady = false;
//			KeeICE::KPlib->ShutdownICE();
		}
		//TODO: is this where we should release some XPCOM proxy memory?
		//KeeICE.ic.destroy();
    }
};

NS_IMETHODIMP CKeeFox::AddObserver(KeeFoxObserver *observer)
{
	nsresult rv = NS_OK;

	// if we've already set this up once for this firefox session just return
	//TODO: more thought needed here. will a ICE callback make it through to the 
	// javascript callback handler before the JS is ready for it? (in the case that 
	// KeeICE was started after KeeFox, or for a 2nd time...)
	if (KeeFoxCallBackHelper::javascriptCallBacksReady)
		return rv;

	// get the proxy manager object
	nsCOMPtr<nsIProxyObjectManager> pIProxyObjectManager =
	do_GetService("@mozilla.org/xpcomproxy;1", &rv);
	NS_ENSURE_SUCCESS(rv, rv);

	// use proxy manager to create a proxy object for the observer we've been given
	// nsnull -> send proxied calls to the main Firefox UI thread
	rv = pIProxyObjectManager->GetProxyForObject( nsnull,
                                                  KeeFoxObserver::GetIID(),
                                                  observer,
                                                  0x0001 | 0x0004,
                                                  (void**)&KeeFoxCallBackHelper::_observer);
	// 0x0001 = synchronous, 0x0002 = async
	
	//TODO: these lines probably need to be uncommented to prevent memory leaks
    // we do not care about the real object anymore. ie. GetProxyObject
    // refcounts it.
	//NS_RELEASE(observer);
	//KeeFoxCallBackHelper::_observer->Test1(x,y,z);
	//NS_RELEASE(KeeFoxCallBackHelper::_observer);


	//KeeFoxCallBackHelper::_observer->SetJavascriptCallBacksReady((PRBool)true);
	KeeFoxCallBackHelper::javascriptCallBacksReady = true;


	// enum for status thoughts:
	//KF_STATUS_JSCALLBACKS_DISABLED 0
	//KF_STATUS_JSCALLBACKS_SETUP 1 // CALL THIS ONE FROM RIGHT HERE AS A QUICK SETUP SANITY TEST
	//KF_STATUS_ICECALLBACKS_SETUP 2
	//KF_STATUS_DATABASE_OPENING 3
	//KF_STATUS_DATABASE_OPEN 4
	//KF_STATUS_DATABASE_CLOSING 5
	//KF_STATUS_DATABASE_CLOSED 6
	//KF_STATUS_DATABASE_SAVING 7
	//KF_STATUS_DATABASE_SAVED 8
	//KF_STATUS_DATABASE_DELETING 9
	//KF_STATUS_DATABASE_DELETED 10
	//KF_STATUS_DATABASE_SELECTED 11
    //KF_STATUS_EXITING 12

	KeeFoxCallBackHelper::_observer->CallBackToKeeFoxJS(1);

	return 0;
}

CKeeFox::CKeeFox()
{
	/* member initializers and constructor code */
	//mName.Assign(L"Default Name");
	//int status = 0;
	//string DBName = "nope";

	
    //app.run(0, NULL);
//	char* argv[1];
//	argv[0] = "no";
//KeeICE.run(0, argv);
    
    
}

CKeeFox::~CKeeFox()
{
	/* destructor code */
	if (KeeICE.ic)
        KeeICE.ic->destroy();
}

int KeeICEProxy::run(int, char*[]) {
	return establishICEConnection();
}


int KeeICEProxy::establishICEConnection() {


        int status = 0;
		//string DBName = "nope";
		
		try {
			nsresult rv;
			nsCOMPtr<nsIPrefService> prefService (do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
			NS_ENSURE_SUCCESS (rv, false);
			nsCOMPtr<nsIPrefBranch> prefBranch;
			rv = prefService->GetBranch("",getter_AddRefs(prefBranch));
			NS_ENSURE_SUCCESS (rv, false);
			PRInt32 port;
			rv= prefBranch->GetIntPref("extensions.chris.tomlinson@keefox.ICE.port",&port);

			if (NS_FAILED(rv) || port == NULL  || port < 1 || port > 65535)
				port = 12535;

			// Get the initialized property set.
			//
			Ice::PropertiesPtr props = Ice::createProperties();

			// Make sure that network and protocol tracing are off.
			//
			props->setProperty("Ice.ACM.Client", "0");
			props->setProperty("Ice.ThreadPool.Client.Size", "2");
			props->setProperty("Ice.ThreadPool.Server.Size", "2");

			//TODO: I suspect that 20 threads are not sufficient for normal operation. Looks like that's the side-effect of various bugs - hopefully squashed now but watch out when trying to drop this down again in v0.7.
			props->setProperty("Ice.ThreadPool.Client.SizeMax", "200");
			props->setProperty("Ice.ThreadPool.Server.SizeMax", "200");

			//props->setProperty("Ice.Trace.Protocol", "0");

			// Initialize a communicator with these properties.
			//
			Ice::InitializationData id;
			id.properties = props;
			ic = Ice::initialize(id);

			//string proxyString = strcat("KeeICE:tcp -h localhost -p ",port);
			char proxyString [50];
			int n = sprintf (proxyString, "KeeICE:tcp -h localhost -p %d", port);

			//ic = Ice::initialize();
			Ice::ObjectPrx base = ic->stringToProxy(proxyString);
			KP = KPPrx::checkedCast(base);
			if (!KP)
				throw "Invalid proxy";

			Ice::ObjectAdapterPtr adapter =	ic->createObjectAdapter("");
			Ice::Identity ident;
			ident.name = IceUtil::generateUUID();
			ident.category = "";
			//CallbackPtr cb = new CallbackI;
			CallbackReceiverPtr cb = new CallbackReceiverI;
			adapter->add(cb, ident);
			adapter->activate();
			KP->ice_getConnection()->setAdapter(adapter);
			KP->addClient(ident);

//    communicator()->waitForShutdown();

			;
		} catch (const Ice::Exception& ex) {
			cerr << ex << endl;
			status = 1;
			if (ic)
				ic->destroy();
		} catch (const char* msg) {
			cerr << msg << endl;
			status = 1;
			if (ic)
				ic->destroy();
		} catch (...) {
			//cerr << ex << endl;
			status = 1;
			if (ic)
				ic->destroy();
		}

        return status;
    }

		/*
int KeeICEProxy::main(int argc, char* argv[])
{
	int status = 0;
		//string DBName = "nope";
		
		try {

			// Get the initialized property set.
			//
			Ice::PropertiesPtr props = Ice::createProperties();

			// Make sure that network and protocol tracing are off.
			//
			props->setProperty("Ice.ACM.Client", "0");
			props->setProperty("Ice.ThreadPool.Client.Size", "2");
			props->setProperty("Ice.ThreadPool.Server.Size", "2");

			//TODO: I suspect that 20 threads are not sufficient for normal operation. Looks like that's the side-effect of various bugs - hopefully squashed now but watch out when trying to drop this down again in v0.7.
			props->setProperty("Ice.ThreadPool.Client.SizeMax", "200");
			props->setProperty("Ice.ThreadPool.Server.SizeMax", "200");

			//props->setProperty("Ice.Trace.Protocol", "0");

			// Initialize a communicator with these properties.
			//
			Ice::InitializationData id;
			id.properties = props;
			ic = Ice::initialize(id);

			//ic = Ice::initialize();
			Ice::ObjectPrx base = ic->stringToProxy(
									"KeeICE:tcp -h localhost -p 12535");
			KP = KPPrx::checkedCast(base);
			if (!KP)
				throw "Invalid proxy";

			Ice::ObjectAdapterPtr adapter =	ic->createObjectAdapter("");
			Ice::Identity ident;
			ident.name = IceUtil::generateUUID();
			ident.category = "";
			//CallbackPtr cb = new CallbackI;
			CallbackReceiverPtr cb = new CallbackReceiverI;
			adapter->add(cb, ident);
			adapter->activate();
			KP->ice_getConnection()->setAdapter(adapter);
			KP->addClient(ident);

//    communicator()->waitForShutdown();

			;
		} catch (const Ice::Exception& ex) {
			cerr << ex << endl;
			status = 1;
			if (ic)
				ic->destroy();
		} catch (const char* msg) {
			cerr << msg << endl;
			status = 1;
			if (ic)
				ic->destroy();
		} catch (...) {
			//cerr << ex << endl;
			status = 1;
			if (ic)
				ic->destroy();
		}

        return status;


*/







  /*  int status = EXIT_SUCCESS;
    Ice::CommunicatorPtr communicator;

    try
    {
        communicator = Ice::initialize(argc, argv);
        if(argc > 1)
        {
            fprintf(stderr, "%s: too many arguments\n", argv[0]);
            return EXIT_FAILURE;
        }
        HelloPrx hello = HelloPrx::checkedCast(communicator->stringToProxy("hello:tcp -p 10000"));
        if(!hello)
        {
            fprintf(stderr, "%s: invalid proxy\n", argv[0]);
            status = EXIT_FAILURE;
        }
        else
        {
            hello->sayHello();
        }
    }
    catch(const Ice::Exception& ex)
    {
        fprintf(stderr, "%s\n", ex.toString().c_str());
        status = EXIT_FAILURE;
    }

    if(communicator)
    {
        try
        {
            communicator->destroy();
        }
        catch(const Ice::Exception& ex)
        {
            fprintf(stderr, "%s\n", ex.toString().c_str());
            status = EXIT_FAILURE;
        }
    }

    return status;*/
//}

NS_IMETHODIMP CKeeFox::LaunchKeePass(nsAString const &fileName, nsAString const &DBFile)
{
	//TODO: this was an example of a random crash! nsString destructor called after already destroyed by something else? maybe another thread's GC? can we be sure it's the params below rather than input params above? if ones below then maybe some alternative way to convert the input vars in a way that we can destroy the objects before passing the native string to the execute method? if former, maybe it's becuase the javascript vars in calling function are defined as locals but that function shouldn't go out of scope before this function has completed...
ShellExecute(NULL,L"open",nsString(fileName).get(), nsString(DBFile).get(),L"",SW_SHOW );
return NS_OK;
}

//TODO: can we grab the return code to discover if the installation was successful?
NS_IMETHODIMP CKeeFox::RunAnInstaller(nsAString const &fileName, nsAString const &params)
{
	if (IsVista())
		RunElevated(NULL, nsString(fileName).get(), nsString(params).get(), NULL );
	else
		if (::IsUserAnAdmin())
			MyShellExec( NULL, L"open", nsString(fileName).get(), nsString(params).get(),L"", TRUE );
		else
			MyShellExec( NULL, L"runas", nsString(fileName).get(), nsString(params).get(),L"", TRUE );

	return NS_OK;
}

NS_IMETHODIMP CKeeFox::IsUserAdministrator(PRBool *_retval)
{
	if (IsVista())
	{
		

		TOKEN_ELEVATION_TYPE ptet;
		HRESULT res = GetElevationType(&ptet);
		if (res == S_OK && ptet != TokenElevationTypeDefault) 
		{
			// user has a split token so must be administrator
			*_retval = true;
		}
		else
		{
			// UAC is disabled or the user has limited rights, we'll try to find out the old fashioned way...
			*_retval = ::IsUserAnAdmin();
		}
	} else
	{
		*_retval = ::IsUserAnAdmin();
	}
	return NS_OK;
}



NS_IMETHODIMP CKeeFox::ShutdownICE()
{
	//TODO: what is going on here? should these three lines be uncommented to remove memory leaks? If so, need to stop everything deadlocking when they execute!
	//KeeICE.ic->shutdown();
	//KeeICE.ic->waitForShutdown(); // destroy? // or assign to null? // look up about killing dead proxies?
	//KeeICE.ic->destroy();
	KeeICE.KP = NULL;
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetDBName(nsAString &_retval)
{
	int status = 0;
	string DBName = "no name";

    try {
		if (!KeeICE.KP)
            status = 1;
		else
			DBName = KeeICE.KP->getDatabaseName();
    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

	if (status)
		return NS_ERROR_NOT_AVAILABLE;

	// Convert response to XPCOM string
	_retval = NS_ConvertUTF8toUTF16(DBName.c_str());
	
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetDBFileName(nsAString &_retval)
{
	int status = 0;
	string DBFileName = "nope";

    try {
		if (!KeeICE.KP)
            status = 1;
		else
			DBFileName = KeeICE.KP->getDatabaseFileName();
    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

	if (status)
		return NS_ERROR_NOT_AVAILABLE;

	// Convert response to XPCOM string
	_retval = NS_ConvertUTF8toUTF16(DBFileName.c_str());
	
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::ChangeDB(nsAString const &fileName, PRBool closeCurrent)
{
	int status = 0;
	//string DBFileName = "nope";

    try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		KeeICE.KP->changeDatabase(NS_ConvertUTF16toUTF8(fileName).get(), closeCurrent != 0);
        ;
    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

	// Convert response to XPCOM string
	//_retval = NS_ConvertUTF8toUTF16(DBFileName.c_str());
	
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::CheckVersion(float keeFoxVersion, float keeICEVersion, PRInt32 *result, PRBool *_retval)
{
	int status = 0;
	*_retval = false;

    try {
		if (!KeeICE.KP)
		{
			status = KeeICE.establishICEConnection();
		}
		//throw "Proxy went away";
		if (!status)
			*_retval = KeeICE.KP->checkVersion(keeFoxVersion, keeICEVersion, *result);
    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

	return NS_OK;
}

/* attribute AString name; */
NS_IMETHODIMP CKeeFox::GetName(nsAString & aName)
{
	aName.Assign(mName);
	return NS_OK;
}
NS_IMETHODIMP CKeeFox::SetName(const nsAString & aName)
{
	mName.Assign(aName);
	return NS_OK;
}





KeeICE::KPlib::KPEntry CKeeFox::ConvertLoginInfoToKPEntry (kfILoginInfo *aLogin)
{
	KeeICE::KPlib::KPEntry entry;
	KPFormFieldList *formFieldList = new KPFormFieldList();
	entry.formFieldList = *formFieldList;

	//nsString URL;
	//(void)aLogin->GetURL(URL);
	//entry.URL = NS_ConvertUTF16toUTF8(URL).get(); 
	nsCOMPtr<nsIMutableArray> URLs;
	(void)aLogin->GetURLs(getter_AddRefs(URLs));
	
	//entry.title = entry.hostName;

	nsString formActionURL;
	(void)aLogin->GetFormActionURL(formActionURL);
	entry.formActionURL = NS_ConvertUTF16toUTF8(formActionURL).get();

	nsString httpRealm;
	(void)aLogin->GetHttpRealm(httpRealm);
	entry.HTTPRealm = NS_ConvertUTF16toUTF8(httpRealm).get(); 

	nsString uniqueID;
	(void)aLogin->GetUniqueID(uniqueID);
	entry.uniqueID = NS_ConvertUTF16toUTF8(uniqueID).get();

	nsString title;
	(void)aLogin->GetTitle(title);
	entry.title = NS_ConvertUTF16toUTF8(title).get();


/*
	nsCOMPtr<kfILoginField> usernameField;
	(void)aLogin->GetUsername(getter_AddRefs(usernameField));

	nsString fieldValue;
	(void)usernameField->GetValue(fieldValue);
	nsString fieldName;
	(void)usernameField->GetName(fieldName);
	nsString fieldID;
	(void)usernameField->GetId(fieldID);

	KPFormField *usernameFF = new KPFormField();
	usernameFF->name = NS_ConvertUTF16toUTF8(fieldName).get(); 
	usernameFF->value = NS_ConvertUTF16toUTF8(fieldValue).get();
	usernameFF->id = NS_ConvertUTF16toUTF8(fieldID).get();
	usernameFF->type = formFieldType::FFTusername;

	entry.formFieldList.push_back(*usernameFF);*/

	PRInt32 usernameIndex;
	(void)aLogin->GetUsernameIndex(&usernameIndex);

	nsCOMPtr<nsIMutableArray> passwordFields;
	(void)aLogin->GetPasswords(getter_AddRefs(passwordFields));

	nsCOMPtr<nsIMutableArray> otherFields;
	(void)aLogin->GetOtherFields(getter_AddRefs(otherFields));

	// get the array
	//nsCOMPtr<nsIArray> array;
	//foo->GetElements(getter_AddRefs(array));
	
	// handle all URLs


//KeeICE::KPlib::KPURLs EntryURLs;

	/*try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		KFConfiguration kfconfig;
		kfconfig = KeeICE.KP->getCurrentKFConfig();
		fileNames = kfconfig.knownDatabases;

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }*/
/*

   char **strings = (char **)NS_Alloc(fileNames.size() * sizeof(char *));
   if (strings == NULL)
      return NS_ERROR_OUT_OF_MEMORY;

   for (int i = 0; i < fileNames.size(); i++)
   {
	  string fileName = fileNames.at(i);
	  int fileNameLength = fileName.size();

      strings[i] = ToNewUTF8String(NS_ConvertUTF8toUTF16(fileName.c_str())); //nsMemory::Alloc(fileNameLength + 1);
      */
	  // If there are errors free all allocated memory
		// and do not change [out] params.
	  /*if (strings[i] == NULL)
      {
         for (int j = 0; j < i; j++)
            nsMemory::Free(strings[j]);

         nsMemory::Free(strings);

         return NS_ERROR_OUT_OF_MEMORY;
      }
      strncpy(strings[i], fileName.c_str(), fileNameLength);*/
  // }
   
   
   // If no errors
   //*count = fileNames.size();
   //*nsDatabases = strings;


	if (URLs != NULL)
	{
		PRUint32 URLCount = 0;
		URLs->GetLength(&URLCount);

		for (PRUint32 i=0; i<URLCount; ++i)
		{
nsCOMPtr<kfIURL> kfURL;
				URLs->QueryElementAt(i, NS_GET_IID(kfIURL),
								getter_AddRefs(kfURL));

				nsString URL;
				(void)kfURL->GetURL(URL);

			//nsString *URL;
			//*URL = URLs->At(i);

			entry.URLs.push_back(NS_ConvertUTF16toUTF8(URL).get());

			//nsString URL;
			//URLs->QueryElementAt(i, NS_GET_IID(kfILoginField),
			//				getter_AddRefs(passwordField));
/*
			nsString URL;
			(void)passwordField->GetValue(fieldValue);
			nsString fieldName;
			(void)passwordField->GetName(fieldName);
			nsString fieldID;
			(void)passwordField->GetFieldId(fieldID);
			PRInt16 *fieldPage;
			(void)passwordField->GetFormFieldPage(fieldPage);

			KPFormField *passwordFF = new KPFormField();
			passwordFF->name = NS_ConvertUTF16toUTF8(fieldName).get(); 
			passwordFF->value = NS_ConvertUTF16toUTF8(fieldValue).get();
			passwordFF->id = NS_ConvertUTF16toUTF8(fieldID).get();
			passwordFF->type = formFieldType::FFTpassword;
			passwordFF->page = *fieldPage;

			entry.formFieldList.push_back(*passwordFF);*/
		}

	}

	// handle all password fields
	if (passwordFields != NULL)
	{
		PRUint32 passwordCount = 0;
		passwordFields->GetLength(&passwordCount);

		//if (passwordCount > 0)
		//{
			//PRUint32 length;
			//passwordFields->GetLength(&length);

			for (PRUint32 i=0; i<passwordCount; ++i)
			{
				nsCOMPtr<kfILoginField> passwordField;
				passwordFields->QueryElementAt(i, NS_GET_IID(kfILoginField),
								getter_AddRefs(passwordField));

				nsString fieldValue;
				(void)passwordField->GetValue(fieldValue);
				nsString fieldName;
				(void)passwordField->GetName(fieldName);
				nsString fieldID;
				(void)passwordField->GetFieldId(fieldID);
				PRInt16 fieldPage;
				(void)passwordField->GetFormFieldPage(&fieldPage);

				KPFormField *passwordFF = new KPFormField();
				passwordFF->name = NS_ConvertUTF16toUTF8(fieldName).get(); 
				passwordFF->value = NS_ConvertUTF16toUTF8(fieldValue).get();
				passwordFF->id = NS_ConvertUTF16toUTF8(fieldID).get();
				passwordFF->type = formFieldType::FFTpassword;
				passwordFF->page = (int)fieldPage;

				entry.formFieldList.push_back(*passwordFF);
			}
		//}
	}

	// handle all other form fields
	if (otherFields != NULL)
	{
		PRUint32 otherCount = 0;
		otherFields->GetLength(&otherCount);

		for (PRUint32 i=0; i<otherCount; ++i)
		{
			nsCOMPtr<kfILoginField> otherField;
			otherFields->QueryElementAt(i, NS_GET_IID(kfILoginField),
							getter_AddRefs(otherField));

			nsString fieldValue;
			(void)otherField->GetValue(fieldValue);
			nsString fieldName;
			(void)otherField->GetName(fieldName);
			nsString fieldID;
			(void)otherField->GetFieldId(fieldID);
			nsString fieldType;
			(void)otherField->GetType(fieldType);
			PRInt16 fieldPage;
			(void)otherField->GetFormFieldPage(&fieldPage);

			KPFormField *otherFF = new KPFormField();
			otherFF->name = NS_ConvertUTF16toUTF8(fieldName).get(); 
			otherFF->value = NS_ConvertUTF16toUTF8(fieldValue).get();
			otherFF->id = NS_ConvertUTF16toUTF8(fieldID).get();
			otherFF->page = (int)fieldPage;
			
			string type = NS_ConvertUTF16toUTF8(fieldType).get();

			if (type == "text")
			{
					if (i == usernameIndex)
						otherFF->type = formFieldType::FFTusername;
					else
						otherFF->type = formFieldType::FFTtext;
			} else if (type == "checkbox")
			{
					otherFF->type = formFieldType::FFTcheckbox;
			} else if (type == "radio")
			{
					otherFF->type = formFieldType::FFTradio;
			} else if (type == "select")
			{
					otherFF->type = formFieldType::FFTselect;
			}

			entry.formFieldList.push_back(*otherFF);
		}
	}

	return entry;
}



nsCOMPtr<kfILoginInfo> CKeeFox::ConvertKPEntryToLoginInfo (KeeICE::KPlib::KPEntry entry)
{
	nsresult rv;
	PRInt32 usernameIndex = -1;
	int maximumPage = 0;

	nsCOMPtr<nsIMutableArray> URLs = do_CreateInstance(NS_ARRAY_CONTRACTID);
	nsCOMPtr<nsIMutableArray> passwordFields = do_CreateInstance(NS_ARRAY_CONTRACTID);
	nsCOMPtr<nsIMutableArray> otherFields = do_CreateInstance(NS_ARRAY_CONTRACTID);
			

   for (int i = 0; i < entry.URLs.size(); i++)
   {

nsCOMPtr<kfIURL> kfURL = do_CreateInstance("@christomlinson.name/kfURL;1");
			if (!kfURL) {
			  return NULL;
			}

			kfURL->SetURL(NS_ConvertUTF8toUTF16(entry.URLs.at(i).c_str()));

		//nsString *kpURL = entry.URLs.at(i);
		URLs->AppendElement(kfURL,false);


	  //string URL = URLs.at(i);
	  //i//nt URLLength = URL.size();

      //strings[i] = ToNewUTF8String(NS_ConvertUTF8toUTF16(kpURL.c_str())); //nsMemory::Alloc(fileNameLength + 1);
     
	  // If there are errors free all allocated memory
		// and do not change [out] params.
	  /*if (strings[i] == NULL)
      {
         for (int j = 0; j < i; j++)
            nsMemory::Free(strings[j]);

         nsMemory::Free(strings);

         return NS_ERROR_OUT_OF_MEMORY;
      }
      strncpy(strings[i], fileName.c_str(), fileNameLength);*/
   }
   
   
   // If no errors
   //*count = fileNames.size();
   //*nsDatabases = strings;










	for (int j = 0; j < entry.formFieldList.size(); j++) 
	{
		KPFormField kpff = entry.formFieldList.at(j);

		#if _DEBUG
		  std::cout << "found a new field..." << "\n";
		#endif
  
		if (kpff.type == formFieldType::FFTpassword)
		{
			nsString passwordValue = NS_ConvertUTF8toUTF16(kpff.value.c_str());
			nsString passwordName = NS_ConvertUTF8toUTF16(kpff.name.c_str());
			nsString passwordID = NS_ConvertUTF8toUTF16(kpff.id.c_str());
			int passwordPage = kpff.page;
			if (passwordPage > maximumPage)
				maximumPage = passwordPage;
			
			nsCOMPtr<kfILoginField> passwordField = do_CreateInstance("@christomlinson.name/kfLoginField;1");
			if (!passwordField) {
			  return NULL;
			}

			rv = passwordField->Init(passwordName, passwordValue, passwordID, NS_ConvertUTF8toUTF16("password"), passwordPage);

			if (NS_SUCCEEDED(rv))
				passwordFields->AppendElement(passwordField,false);

				#if _DEBUG
				  std::cout << "found a password" << "\n";
				#endif
		} else if (kpff.type == formFieldType::FFTtext || kpff.type == formFieldType::FFTusername
			 || kpff.type == formFieldType::FFTselect || kpff.type == formFieldType::FFTradio
			  || kpff.type == formFieldType::FFTcheckbox)
		{
			PRUint32 otherLength = 0;

			otherFields->GetLength(&otherLength);

			string type = "unknown";

			switch (kpff.type)
			{
				case formFieldType::FFTusername: usernameIndex = otherLength; type = "username"; break;
				case formFieldType::FFTtext: type = "text"; break;
				case formFieldType::FFTradio: type = "radio"; break;
				case formFieldType::FFTcheckbox: type = "checkbox"; break;
				case formFieldType::FFTselect: type = "select"; break;
			}

			nsString otherValue = NS_ConvertUTF8toUTF16(kpff.value.c_str());
			nsString otherName = NS_ConvertUTF8toUTF16(kpff.name.c_str());
			nsString otherID = NS_ConvertUTF8toUTF16(kpff.id.c_str());
			nsString otherType = NS_ConvertUTF8toUTF16(type.c_str());
			int otherPage = kpff.page;
			if (otherPage > maximumPage)
				maximumPage = otherPage;
			
			nsCOMPtr<kfILoginField> otherField = do_CreateInstance("@christomlinson.name/kfLoginField;1");
			if (!otherField) {
			  return NULL;
			}

			rv = otherField->Init(otherName, otherValue, otherID, otherType, otherPage);

			if (NS_SUCCEEDED(rv))
				otherFields->AppendElement(otherField,false);

			#if _DEBUG
			  std::cout << "found a text/other field" << "\n";
			#endif
		} else
		{
			#if _DEBUG
			  std::cout << "not a username, password or text/custom field - unsupported." << "\n";
			#endif
		}
	}

	nsString NSURL,NSactionURL,NSHTTPRealm,NSuniqueID,NStitle;
	PRInt32 NSpages;

	NSactionURL = NS_ConvertUTF8toUTF16(entry.formActionURL.c_str());
	NSHTTPRealm = NS_ConvertUTF8toUTF16(entry.HTTPRealm.c_str());
	NSuniqueID = NS_ConvertUTF8toUTF16(entry.uniqueID.c_str());
	NStitle = NS_ConvertUTF8toUTF16(entry.title.c_str());
	NSpages = maximumPage;

	nsCOMPtr<kfILoginInfo> info = do_CreateInstance("@christomlinson.name/kfLoginInfo;1");
	if (!info) {
	  return NULL;
	}

		rv = info->Init(URLs, NSactionURL, NSHTTPRealm, usernameIndex, passwordFields, NSuniqueID, NStitle, otherFields, NSpages);

	if (NS_SUCCEEDED(rv))
	  return info;
}




KeeICE::KPlib::KPGroup CKeeFox::ConvertGroupInfoToKPGroup (kfIGroupInfo *aGroup)
{
	KeeICE::KPlib::KPGroup group;
	
	nsString name;
	(void)aGroup->GetTitle(name);
	group.title = NS_ConvertUTF16toUTF8(name).get(); 

	nsString uniqueID;
	(void)aGroup->GetUniqueID(uniqueID);
	group.uniqueID = NS_ConvertUTF16toUTF8(uniqueID).get();

	return group;
}



nsCOMPtr<kfIGroupInfo> CKeeFox::ConvertKPGroupToGroupInfo (KeeICE::KPlib::KPGroup group)
{
	nsresult rv;
	nsString name, uniqueRef;
	nsString NSname, NSuniqueID;

	NSname = NS_ConvertUTF8toUTF16(group.title.c_str());
	NSuniqueID = NS_ConvertUTF8toUTF16(group.uniqueID.c_str());

	nsCOMPtr<kfIGroupInfo> info = do_CreateInstance("@christomlinson.name/kfGroupInfo;1");
	if (!info) {
	  return NULL;
	}

	rv = info->Init(NSname, NSuniqueID);

	if (NS_SUCCEEDED(rv))
	  return info;
}





NS_IMETHODIMP CKeeFox::AddLogin(kfILoginInfo *aLogin, const nsAString &parentUUID, kfILoginInfo **aNewLogin)
{
  NS_ENSURE_ARG_POINTER(aLogin);

#if _DEBUG
  std::cout << "comp-impl.cpp::AddLogin - start" << "\n";
#endif

KeeICE::KPlib::KPEntry entry;
entry = ConvertLoginInfoToKPEntry(aLogin);

  try {
	if (!KeeICE.KP)
        throw "Proxy went away";

		entry = KeeICE.KP->AddLogin(entry, NS_ConvertUTF16toUTF8(parentUUID).get());

		// does this work with mutable array? mem overflow?
	  kfILoginInfo *retval = (kfILoginInfo *)NS_Alloc(sizeof(kfILoginInfo *));
	  //for (PRInt32 i = 0; i < results.Count(); i++)
		NS_ADDREF(retval = ConvertKPEntryToLoginInfo(entry));
	  *aNewLogin = retval;

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

#if _DEBUG
  std::cout << "comp-impl.cpp::AddLogin - finished" << "\n";
#endif

  return NS_OK;
}


NS_IMETHODIMP CKeeFox::DeleteLogin(const nsAString &uniqueID, PRBool *_retval)
{
	try {
	if (!KeeICE.KP)
        throw "Proxy went away";

	*_retval = KeeICE.KP->removeEntry(NS_ConvertUTF16toUTF8(uniqueID).get());

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::ModifyLogin(kfILoginInfo *aOldLogin, kfILoginInfo *aNewLogin)
{
  NS_ENSURE_ARG_POINTER(aOldLogin);
  NS_ENSURE_ARG_POINTER(aNewLogin);

#if _DEBUG
  std::cout << "comp-impl.cpp::ModifyLogin - start" << "\n";
#endif

KeeICE::KPlib::KPEntry oldEntry;
oldEntry = ConvertLoginInfoToKPEntry(aOldLogin);
KeeICE::KPlib::KPEntry newEntry;
newEntry = ConvertLoginInfoToKPEntry(aNewLogin);

  try {
	if (!KeeICE.KP)
        throw "Proxy went away";

		KeeICE.KP->ModifyLogin(oldEntry, newEntry);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

#if _DEBUG
  std::cout << "comp-impl.cpp::ModifyLogin - finished" << "\n";
#endif

  return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetAllLogins(PRUint32 *count, kfILoginInfo ***logins)
{

#if _DEBUG
  std::cout << "comp-impl.cpp::GetAllLogins - started" << "\n";
#endif

  int status = 0;
  nsresult rv;

  nsCOMArray<kfILoginInfo> results;

  KeeICE::KPlib::KPEntryList entries;

  try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		*count = KeeICE.KP->getAllLogins(entries);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "we have received " << entries.size() << " KPEntry objects" << "\n";
#endif

	for (unsigned int i = 0; i < entries.size(); i++) {
		KPEntry entry = entries.at(i);

	#if _DEBUG
	  std::cout << "found a new KPEntry" << "\n";
	#endif

	  nsCOMPtr<kfILoginInfo> info;

		info = ConvertKPEntryToLoginInfo(entry);
		if (info)
			(void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfILoginInfo object" << "\n";
	#endif
	}

  if (0 == results.Count()) {
    *logins = nsnull;
    *count = 0;
    return NS_OK;
  }
  
  kfILoginInfo **retval = (kfILoginInfo **)NS_Alloc(sizeof(kfILoginInfo *) * results.Count());
  for (PRInt32 i = 0; i < results.Count(); i++)
    NS_ADDREF(retval[i] = results[i]);
  *logins = retval;
  *count = results.Count();
  
#if _DEBUG
  std::cout << "count: " << *count << "\n";
#endif

#if _DEBUG
  std::cout << "comp-impl.cpp::GetAllLogins - finished" << "\n";
#endif

  return NS_OK;

}

NS_IMETHODIMP CKeeFox::FindLogins(PRUint32 *count, const nsAString &aHostname, const nsAString &aActionURL, const nsAString &aHttpRealm, const nsAString &aUniqueID, kfILoginInfo ***logins)
{
 
#if _DEBUG
  std::cout << "comp-impl.cpp::FindLogins - started" << "\n";
#endif

  int status = 0;
  

  nsCOMArray<kfILoginInfo> results;

  KeeICE::KPlib::KPEntryList entries;

  try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		// null (Void) strings and empty strings are different in Firefox but ICE can't support that so there are
		// various places like this where we need to make some decisions about what KeePass can return before
		// any message is actually sent to KeePass. A bit counter-intuitive but it works and reduces
		// inter-application traffic too as a nice side-effect.
		if (aActionURL.IsVoid() && aHttpRealm.IsVoid())
		{	
			*count = 0;
			#if _DEBUG
			  std::cout << "comp-impl.cpp::FindLogins - finished" << "\n";
			#endif
			return NS_OK;
		}

		string hostname = NS_ConvertUTF16toUTF8(aHostname).get();
		string actionURL = NS_ConvertUTF16toUTF8(aActionURL).get();
		string httpRealm = NS_ConvertUTF16toUTF8(aHttpRealm).get();
		string uniqueID = NS_ConvertUTF16toUTF8(aUniqueID).get();

		

		if (aHttpRealm.IsVoid())
			*count = KeeICE.KP->findLogins(hostname,actionURL,httpRealm,KeeICE::KPlib::loginSearchType(LSTnoRealms),false, uniqueID, entries);
		else if (aActionURL.IsVoid())
			*count = KeeICE.KP->findLogins(hostname,actionURL,httpRealm,KeeICE::KPlib::loginSearchType(LSTnoForms),false, uniqueID, entries);
		else
			*count = KeeICE.KP->findLogins(hostname,actionURL,httpRealm,KeeICE::KPlib::loginSearchType(LSTall), false, uniqueID, entries);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "we have received " << entries.size() << " KPEntry objects" << "\n";
#endif

	for (unsigned int i = 0; i < entries.size(); i++) {
		KPEntry entry = entries.at(i);

	#if _DEBUG
	  std::cout << "found a new KPEntry" << "\n";
	#endif

	  nsCOMPtr<kfILoginInfo> info;

		info = ConvertKPEntryToLoginInfo(entry);
		if (info)
			(void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfILoginInfo object" << "\n";
	#endif
	}

  if (0 == results.Count()) {
    *logins = nsnull;
    *count = 0;
#if _DEBUG
	std::cout << "count: " << *count << "\n";
	std::cout << "comp-impl.cpp::FindLogins - finished" << "\n";
#endif
    return NS_OK;
  }
  
  // does this work with mutable array? mem overflow?
  kfILoginInfo **retval = (kfILoginInfo **)NS_Alloc(sizeof(kfILoginInfo *) * results.Count());
  for (PRInt32 i = 0; i < results.Count(); i++)
    NS_ADDREF(retval[i] = results[i]);
  *logins = retval;
  *count = results.Count();
  
#if _DEBUG
  std::cout << "count: " << *count << "\n";
#endif

#if _DEBUG
  std::cout << "comp-impl.cpp::FindLogins - finished" << "\n";
#endif

  return NS_OK;
}

NS_IMETHODIMP CKeeFox::CountLogins(nsAString const &aHostname, nsAString const &aActionURL,
	nsAString const &aHttpRealm, unsigned int *_retval)
{
#if _DEBUG
  std::cout << "comp-impl.cpp::CountLogins - started" << "\n";
#endif

	int status = 0;
	int count = 0;

    try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		// null (Void) strings and empty strings are different in Firefox but ICE can't support that concept
		// so there are various places like this where we need to make some decisions about what KeePass
		// can return before any message is actually sent to KeePass. A bit counter-intuitive but
		// it works and reduces inter-application traffic too as a nice side-effect.
		if (aActionURL.IsVoid() && aHttpRealm.IsVoid())
		{	
			*_retval = 0;
			#if _DEBUG
			  std::cout << "comp-impl.cpp::CountLogins - finished" << "\n";
			#endif
			return NS_OK;
		}

		string hostname = NS_ConvertUTF16toUTF8(aHostname).get();
		string actionURL = NS_ConvertUTF16toUTF8(aActionURL).get();
		string httpRealm = NS_ConvertUTF16toUTF8(aHttpRealm).get();

		if (aHttpRealm.IsVoid())
			*_retval = KeeICE.KP->countLogins(hostname,actionURL,httpRealm,KeeICE::KPlib::loginSearchType(LSTnoRealms), false);
		else if (aActionURL.IsVoid())
			*_retval = KeeICE.KP->countLogins(hostname,actionURL,httpRealm,KeeICE::KPlib::loginSearchType(LSTnoForms), false);
		else
			*_retval = KeeICE.KP->countLogins(hostname,actionURL,httpRealm,KeeICE::KPlib::loginSearchType(LSTall), false);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "comp-impl.cpp::CountLogins - finished" << "\n";
#endif
//*_retval = 1;
return NS_OK;
}




NS_IMETHODIMP CKeeFox::FindGroups(PRUint32 *count, const nsAString &aName, const nsAString &aUniqueID, kfIGroupInfo ***logins)
{
 
#if _DEBUG
  std::cout << "comp-impl.cpp::FindGroups - started" << "\n";
#endif

  int status = 0;
  
  nsCOMArray<kfIGroupInfo> results;

  KeeICE::KPlib::KPGroupList groups;

  try {
		if (!KeeICE.KP)
            throw "Proxy went away";

		string name = NS_ConvertUTF16toUTF8(aName).get();
		string uniqueID = NS_ConvertUTF16toUTF8(aUniqueID).get();

		*count = KeeICE.KP->findGroups(name, uniqueID, groups);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
		status = 1;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
        status = 1;
    } catch (const char* msg) {
        cerr << msg << endl;
        status = 1;
	} catch (...) {
        status = 1;
    }

#if _DEBUG
  std::cout << "we have received " << groups.size() << " KPGroup objects" << "\n";
#endif

	for (unsigned int i = 0; i < groups.size(); i++) {
		KPGroup group = groups.at(i);

	#if _DEBUG
	  std::cout << "found a new KPGroup" << "\n";
	#endif

	  nsCOMPtr<kfIGroupInfo> info;

		info = ConvertKPGroupToGroupInfo(group);
		if (info)
			(void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfIGroupInfo object" << "\n";
	#endif
	}

  if (0 == results.Count()) {
    *logins = nsnull;
    *count = 0;
#if _DEBUG
	std::cout << "count: " << *count << "\n";
	std::cout << "comp-impl.cpp::FindGroups - finished" << "\n";
#endif
    return NS_OK;
  }
  
  // does this work with mutable array? mem overflow?
  kfIGroupInfo **retval = (kfIGroupInfo **)NS_Alloc(sizeof(kfIGroupInfo *) * results.Count());
  for (PRInt32 i = 0; i < results.Count(); i++)
    NS_ADDREF(retval[i] = results[i]);
  *logins = retval;
  *count = results.Count();
  
#if _DEBUG
  std::cout << "count: " << *count << "\n";
#endif

#if _DEBUG
  std::cout << "comp-impl.cpp::FindGroups - finished" << "\n";
#endif

  return NS_OK;
}



NS_IMETHODIMP CKeeFox::AddGroup(const nsAString &name, const nsAString &parentUUID, kfIGroupInfo **aGroup)
{

#if _DEBUG
  std::cout << "comp-impl.cpp::AddGroup - start" << "\n";
#endif

KeeICE::KPlib::KPGroup group;

  try {
	if (!KeeICE.KP)
        throw "Proxy went away";

		group = KeeICE.KP->addGroup(NS_ConvertUTF16toUTF8(name).get(), NS_ConvertUTF16toUTF8(parentUUID).get());

		// does this work with mutable array? mem overflow?
		kfIGroupInfo *retval = (kfIGroupInfo *)NS_Alloc(sizeof(kfIGroupInfo *));
		//for (PRInt32 i = 0; i < results.Count(); i++)
		NS_ADDREF(retval = ConvertKPGroupToGroupInfo(group));
		*aGroup = retval;

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

#if _DEBUG
  std::cout << "comp-impl.cpp::AddGroup - finished" << "\n";
#endif

  return NS_OK;
}

NS_IMETHODIMP CKeeFox::DeleteGroup(const nsAString &uniqueID, PRBool *_retval)
{
	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		*_retval = KeeICE.KP->removeGroup(NS_ConvertUTF16toUTF8(uniqueID).get());

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}


NS_IMETHODIMP CKeeFox::GetParentGroup(const nsAString &uniqueID, kfIGroupInfo **aGroup)
{
	KeeICE::KPlib::KPGroup group;

	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		group = KeeICE.KP->getParent(NS_ConvertUTF16toUTF8(uniqueID).get());
		
		kfIGroupInfo *retval = (kfIGroupInfo *)NS_Alloc(sizeof(kfIGroupInfo *));
		NS_ADDREF(retval = ConvertKPGroupToGroupInfo(group));
	    *aGroup = retval;

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

	return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetRootGroup(kfIGroupInfo **aGroup)
{
	//std::cout << "comp-impl.cpp::GetRootGroup - started" << "\n";

	KeeICE::KPlib::KPGroup group;

	try {
		group = KeeICE.KP->getRoot(); // is group going out of scope / being GCd?
			
		} catch (const Ice::Exception& ex) {
			cout << ex << endl;
		} catch (const char* msg) {
			cout << msg << endl;
		} catch (...) {
			cout << "getRoot exception :-(" << endl;
		}

	kfIGroupInfo *retval = (kfIGroupInfo *)NS_Alloc(sizeof(kfIGroupInfo *));
    NS_ADDREF(retval = ConvertKPGroupToGroupInfo(group));
  *aGroup = retval;

  //std::cout << "comp-impl.cpp::GetRootGroup - finished" << "\n";

	return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetChildGroups(PRUint32 *count, const nsAString &uniqueID, kfIGroupInfo ***nsGroups)
{
#if _DEBUG
  std::cout << "comp-impl.cpp::GetChildGroups - started" << "\n";
#endif

	nsCOMArray<kfIGroupInfo> results;

	KeeICE::KPlib::KPGroupList groups;

	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		groups = KeeICE.KP->getChildGroups(NS_ConvertUTF16toUTF8(uniqueID).get());

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

	for (unsigned int i = 0; i < groups.size(); i++) {
		KPGroup group = groups.at(i);

	#if _DEBUG
	  std::cout << "found a new KPGroup" << "\n";
	#endif

	  nsCOMPtr<kfIGroupInfo> info;

		info = ConvertKPGroupToGroupInfo(group);
		if (info)
			(void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfIGroupInfo object" << "\n";
	#endif
	}

	kfIGroupInfo **retval = (kfIGroupInfo **)NS_Alloc(sizeof(kfIGroupInfo *) * results.Count());
	for (PRInt32 i = 0; i < results.Count(); i++)
	NS_ADDREF(retval[i] = results[i]);
	*nsGroups = retval;
	*count = results.Count();
	  
	#if _DEBUG
	  std::cout << "count: " << *count << "\n";
	#endif

	#if _DEBUG
	  std::cout << "comp-impl.cpp::GetChildGroups - finished" << "\n";
	#endif

	return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetChildEntries(PRUint32 *count, const nsAString &uniqueID, kfILoginInfo ***nsLogins)
{
#if _DEBUG
  std::cout << "comp-impl.cpp::GetChildEntries - started" << "\n";
#endif

	nsCOMArray<kfILoginInfo> results;

	KeeICE::KPlib::KPEntryList logins;

	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		string uid = NS_ConvertUTF16toUTF8(uniqueID).get();
		logins = KeeICE.KP->getChildEntries(uid);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }

	for (unsigned int i = 0; i < logins.size(); i++) {
		KPEntry login = logins.at(i);

	#if _DEBUG
	  std::cout << "found a new KPEntry" << "\n";
	#endif

	  nsCOMPtr<kfILoginInfo> info;

		info = ConvertKPEntryToLoginInfo(login);
		if (info)
			(void)results.AppendObject(info);

	#if _DEBUG
	  std::cout << "appended new kfILoginInfo object" << "\n";
	#endif
	}

	kfILoginInfo **retval = (kfILoginInfo **)NS_Alloc(sizeof(kfILoginInfo *) * results.Count());
	for (PRInt32 i = 0; i < results.Count(); i++)
	NS_ADDREF(retval[i] = results[i]);
	*nsLogins = retval;
	*count = results.Count();
	  
	#if _DEBUG
	  std::cout << "count: " << *count << "\n";
	#endif

	#if _DEBUG
	  std::cout << "comp-impl.cpp::GetChildEntries - finished" << "\n";
	#endif

	return NS_OK;
}

NS_IMETHODIMP CKeeFox::LaunchGroupEditor(const nsAString &uniqueID)
{
	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		KeeICE.KP->LaunchGroupEditor(NS_ConvertUTF16toUTF8(uniqueID).get());

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::LaunchLoginEditor(const nsAString &uniqueID)
{
	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		KeeICE.KP->LaunchLoginEditor(NS_ConvertUTF16toUTF8(uniqueID).get());

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::GetMRUdatabases(PRUint32 *count, char ***nsDatabases)
{
#if _DEBUG
  std::cout << "comp-impl.cpp::GetMRUdatabases - started" << "\n";
#endif

   NS_ENSURE_ARG_POINTER(count);
   NS_ENSURE_ARG_POINTER(nsDatabases);

//nsCOMArray<nsAString> results;

	KeeICE::KPlib::KPDatabaseFileNameList fileNames;

	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		KFConfiguration kfconfig;
		kfconfig = KeeICE.KP->getCurrentKFConfig();
		fileNames = kfconfig.knownDatabases;

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }


   char **strings = (char **)NS_Alloc(fileNames.size() * sizeof(char *));
   if (strings == NULL)
      return NS_ERROR_OUT_OF_MEMORY;

   for (int i = 0; i < fileNames.size(); i++)
   {
	  string fileName = fileNames.at(i);
	  int fileNameLength = fileName.size();

      strings[i] = ToNewUTF8String(NS_ConvertUTF8toUTF16(fileName.c_str())); //nsMemory::Alloc(fileNameLength + 1);
      
	  // If there are errors free all allocated memory
		// and do not change [out] params.
	  /*if (strings[i] == NULL)
      {
         for (int j = 0; j < i; j++)
            nsMemory::Free(strings[j]);

         nsMemory::Free(strings);

         return NS_ERROR_OUT_OF_MEMORY;
      }
      strncpy(strings[i], fileName.c_str(), fileNameLength);*/
   }
   
   
   // If no errors
   *count = fileNames.size();
   *nsDatabases = strings;

	#if _DEBUG
	  std::cout << "count: " << *count << "\n";
	#endif

	#if _DEBUG
	  std::cout << "comp-impl.cpp::GetMRUdatabases - finished" << "\n";
	#endif

	return NS_OK;


}

NS_IMETHODIMP CKeeFox::GetAutoCommit(PRBool *_retval)
{
	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		KFConfiguration kfconfig;
		kfconfig = KeeICE.KP->getCurrentKFConfig();
		*_retval = kfconfig.autoCommit;

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::SetAutoCommit(const PRBool autoCommit, PRBool *_retval)
{
	try {
		if (!KeeICE.KP)
			throw "Proxy went away";
		
		KFConfiguration kfconfig;
		kfconfig.autoCommit = autoCommit;
		*_retval = KeeICE.KP->setCurrentKFConfig(kfconfig);

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}

NS_IMETHODIMP CKeeFox::SetCurrentDBRootGroup(const nsAString &uniqueID, PRBool *_retval)
{
	try {
		if (!KeeICE.KP)
			throw "Proxy went away";

		*_retval = KeeICE.KP->setCurrentDBRootGroup(NS_ConvertUTF16toUTF8(uniqueID).get());

    } catch (KeeICE::KPlib::KeeICEException ex) {
		cerr << ex.reason << endl;
    } catch (const Ice::Exception& ex) {
        cerr << ex << endl;
    } catch (const char* msg) {
        cerr << msg << endl;
	} catch (...) {
    }
	return NS_OK;
}
