/*
KeeFox - Allows Firefox to communicate with KeePass (via the KeeICE KeePass-plugin)
Copyright 2008-2009 Chris Tomlinson <keefox@christomlinson.name>
  
This install.js file helps manage the installation of .NET, KeePass and KeeICE.

See install.xul for a description of each of the ICs (Install Cases)

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

const KF_KPZIP_DOWNLOAD_PATH = "http://christomlinson.name/dl/";
const KF_KPZIP_FILE_NAME = "KeePass-2.08.zip";
const KF_KPZIP_FILE_CHECKSUM = "";
const KF_KP_DOWNLOAD_PATH = "http://christomlinson.name/dl/";
const KF_KP_FILE_NAME = "KeePass-2.08-Setup.exe";
const KF_KP_FILE_CHECKSUM = "";
const KF_NET_DOWNLOAD_PATH = "http://christomlinson.name/dl/";
const KF_NET_FILE_NAME = "dotnetfx.exe"
const KF_NET_FILE_CHECKSUM = "";
const KF_NET35_DOWNLOAD_PATH = "http://christomlinson.name/dl/";
const KF_NET35_FILE_NAME = "dotnetfx35setup.exe";
const KF_NET35_FILE_CHECKSUM = "";
const KF_KI_FILE_NAME = "KeeICECopier.exe";

const KF_INSTALL_STATE_SELECTED_PRI = 1; // user has asked to run default installation routine
const KF_INSTALL_STATE_NET_DOWNLOADING = 2;
const KF_INSTALL_STATE_NET_DOWNLOADED = 4;
const KF_INSTALL_STATE_NET_EXECUTING = 8;
const KF_INSTALL_STATE_NET_EXECUTED = 16; // .NET installer has been executed or .NET was already installed
const KF_INSTALL_STATE_KP_DOWNLOADING = 32;
const KF_INSTALL_STATE_KP_DOWNLOADED = 64;
const KF_INSTALL_STATE_KP_EXECUTING = 128;
const KF_INSTALL_STATE_KP_EXECUTED = 256;  // KeePass installer has been executed or KeePass was already installed
const KF_INSTALL_STATE_KI_DOWNLOADING = 512;
const KF_INSTALL_STATE_KI_DOWNLOADED = 1024; // KeeICE files have been downloaded (actually we will bundle them with XPI for time being at least)
const KF_INSTALL_STATE_KI_EXECUTING = 2048; // KeeICE files are being copied
const KF_INSTALL_STATE_KI_EXECUTED = 4096;  // KeeICE files have been copied

const KF_INSTALL_STATE_SELECTED_SEC = 8192; // user has asked to run secondary installation routine
const KF_INSTALL_STATE_SELECTED_TER = 16384; // user has asked to run tertiary installation routine
const KF_INSTALL_STATE_NET35_DOWNLOADING = 32768;
const KF_INSTALL_STATE_NET35_DOWNLOADED = 65536;
const KF_INSTALL_STATE_NET35_EXECUTING = 131072;
const KF_INSTALL_STATE_NET35_EXECUTED = 262144; // .NET35 installer has been executed
const KF_INSTALL_STATE_KPZIP_DOWNLOADING = 524288;
const KF_INSTALL_STATE_KPZIP_DOWNLOADED = 1048576;
const KF_INSTALL_STATE_KPZIP_EXECUTING = 2097152;
const KF_INSTALL_STATE_KPZIP_EXECUTED = 4194304;  // KeePass zip has been extracted

var installState = 0;

var setupKPThread;
var setupNETThread;

var mainWin = window.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
.getInterface(Components.interfaces.nsIWebNavigation)
.QueryInterface(Components.interfaces.nsIDocShellTreeItem)
.rootTreeItem
.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
.getInterface(Components.interfaces.nsIDOMWindow);

var mainWindow = mainWin.keeFoxILM._currentWindow;

function prepareInstallPage() {

    // One of 6 installation options. Although it could probably be inferred
    // from the use of installation state flags, I want to know where we are
    // starting from because the user only needs to see information relevant
    // to their starting state rather than experiencing a walk through the
    // entire state machine.
    var installCase; 
    
    var keePassLocation = "not installed";

//TODO: prevent reinstallation if KeeFox is already working
//TODO: (0.7) support upgrades of KP and KI

// only let this install script run once per firefox session
//TODO: try to find out why this is needed so frequently (session restore / loadOnceInTab should keep it to one instance)
//TODO: reset the flag in some cases (e.g. setup.exe cancelled) so user can begin again without FF restart
if (mainWindow.keeFoxInst._keeFoxStorage.get("KFinstallProcessStarted", false))
{
    document.getElementById('KFInstallAlreadyInProgress').setAttribute('hidden', false);
    return;
}
mainWindow.keeFoxInst._keeFoxStorage.set("KFinstallProcessStarted",true);

    if (mainWindow.keeFoxInst._keeFoxExtension.prefs.has("keePassInstalledLocation")) {
        keePassLocation = mainWindow.keeFoxInst._keeFoxExtension.prefs.getValue("keePassInstalledLocation", "not installed");
        if (keePassLocation == "")
            keePassLocation = "not installed";
    }
    if (userHasAdminRights(mainWindow))
        if (keePassLocation != "not installed")
            installCase = 3;
        else if (checkDotNetFramework(mainWindow))
            installCase = 2;
        else
            installCase = 1;
    else
        if (keePassLocation != "not installed")
            installCase = 6;
        else if (checkDotNetFramework(mainWindow))
            installCase = 5;
        else
            installCase = 4;

    mainWindow.KFLog.info("applying installation case " + installCase);
 
    // comment this out to enable normal operation or uncomment to specify a test case
    //installCase = 5;

// configure the current installation state and trigger any relevant pre-emptive file downloads to give the impression of speed to end user
//TODO: support cancellation of pre-emptive downloading in case advanced user chooses unusual option
    switch (installCase) {
        case 1:
            showSection('setupExeInstallButtonMain');
            showSection('adminNETInstallExpander');
            installState = KF_INSTALL_STATE_NET_DOWNLOADING | KF_INSTALL_STATE_KI_DOWNLOADED;
            mainWindow.KFdownloadFile("IC1PriDownload", KF_NET_DOWNLOAD_PATH + KF_NET_FILE_NAME, KF_NET_FILE_NAME, mainWindow, window);
            break;
        case 2: 
            showSection('KPsetupExeSilentInstallButtonMain');
            showSection('adminSetupKPInstallExpander');
            installState = KF_INSTALL_STATE_NET_EXECUTED | KF_INSTALL_STATE_KP_DOWNLOADING | KF_INSTALL_STATE_KI_DOWNLOADED;
            mainWindow.KFdownloadFile("IC2PriDownload", KF_KP_DOWNLOAD_PATH + KF_KP_FILE_NAME, KF_KP_FILE_NAME, mainWindow, window);
            break;
        case 3: 
            showSection('copyKIToKnownKPLocationInstallButtonMain');
            installState = KF_INSTALL_STATE_NET_EXECUTED | KF_INSTALL_STATE_KP_EXECUTED | KF_INSTALL_STATE_KI_DOWNLOADED;
            break;
        case 4: 
            showSection('setupExeInstallButtonMain');
            showSection('nonAdminNETInstallExpander');
            installState = KF_INSTALL_STATE_NET_DOWNLOADING | KF_INSTALL_STATE_KI_DOWNLOADED;
            mainWindow.KFdownloadFile("IC1PriDownload", KF_NET_DOWNLOAD_PATH + KF_NET_FILE_NAME, KF_NET_FILE_NAME, mainWindow, window); // using IC1 since same process is followed from now on...
            break;
        case 5: 
            showSection('copyKPToSpecificLocationInstallButtonMain'); 
            showSection('nonAdminSetupKPInstallExpander');
            installState = KF_INSTALL_STATE_NET_EXECUTED | KF_INSTALL_STATE_KPZIP_DOWNLOADING | KF_INSTALL_STATE_KI_DOWNLOADED;
            mainWindow.KFdownloadFile("IC5PriDownload", KF_KPZIP_DOWNLOAD_PATH + KF_KPZIP_FILE_NAME, KF_KPZIP_FILE_NAME, mainWindow, window);
            break;
        case 6: 
            showSection('copyKIToKnownKPLocationInstallButtonMain');
            showSection('nonAdmincopyKIToKnownKPLocationInstallExpander');
            installState = KF_INSTALL_STATE_NET_EXECUTED | KF_INSTALL_STATE_KP_EXECUTED | KF_INSTALL_STATE_KI_DOWNLOADED;
            break;            
        default: document.getElementById('ERRORInstallButtonMain').setAttribute('hidden', false); break;
    }
}

function hideInstallView() {
    document.getElementById('installationStartView').setAttribute('hidden', true);
}

function showProgressView() {
    document.getElementById('installProgressView').setAttribute('hidden', false);
}

function showSection(id) {
    document.getElementById(id).setAttribute('hidden', false);
}

function hideSection(id) {
    document.getElementById(id).setAttribute('hidden', true);
}

/********************
START:
functions to support the execution of Install Case 1
********************/
function IC1finished(mainWindow) {

    hideSection('IC1KIdownloaded');
    showSection('InstallFinished');
    
    launchAndConnectToKeePass();
}

function IC1setupKI(mainWindow) {
try {
    if ((installState & KF_INSTALL_STATE_KI_DOWNLOADED) && (installState & KF_INSTALL_STATE_KP_EXECUTED))
    {
        // we don't checksum bundled DLLs since if they are compromised the whole XPI, including this file, could be too
        hideSection('IC1setupKPdownloaded');
        showSection('IC1KIdownloaded');
        
        installState |= KF_INSTALL_STATE_KI_EXECUTING;

        // we've just run the official KeePass installer so can be pretty confident that we can automatically find the installation directory (though if we get time, we should handle exceptions too...)
        var keePassLocation;
        var KeePassEXEfound;
        KeePassEXEfound = false;
        keePassLocation = "not installed";

        keePassLocation = mainWindow.keeFoxInst._discoverKeePassInstallLocation();
        if (keePassLocation != "not installed")
        {
            KeePassEXEfound = mainWindow.keeFoxInst._confirmKeePassInstallLocation(keePassLocation);
        }
        
        if (KeePassEXEfound)
        {
            copyKeeICEFilesTo(keePassLocation);

            installState ^= KF_INSTALL_STATE_KI_EXECUTING; // we can safely assume this bit is already set since no other threads are involved
            installState |= KF_INSTALL_STATE_KI_EXECUTED;
            IC1finished(mainWindow);
        } else
        {
            hideSection('IC1KIdownloaded');
            showSection('ERRORInstallButtonMain');
        }
    }
    } catch (err) {
            Components.utils.reportError(err);
        }
}

function IC1setupKP(mainWindow) {
try {

    if ((installState & KF_INSTALL_STATE_KP_DOWNLOADED) && (installState & KF_INSTALL_STATE_NET_EXECUTED || installState & KF_INSTALL_STATE_NET35_EXECUTED))
    {
        if (installState & KF_INSTALL_STATE_NET_EXECUTED)
            hideSection('IC1setupNETdownloaded');
        else if (installState & KF_INSTALL_STATE_NET35_EXECUTED)
            hideSection('IC1setupNET35downloaded');
            
        var checkTest = mainWindow.KFMD5checksumVerification(KF_KP_FILE_NAME,KF_KP_FILE_CHECKSUM);
        
        if (checkTest)
            mainWindow.KFLog.info("File checksum succeeded.");
        else
            mainWindow.KFLog.error("File checksum failed. Download corrupted?");
        // TODO: kick user back to start if it fails
        
        hideSection('IC1setupKPdownloading'); // if applicable (probably this was never shown)
        showSection('IC1setupKPdownloaded');
        
        var file = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        file.append(KF_KP_FILE_NAME);
        
        threadFixer = file.path;

        installState |= KF_INSTALL_STATE_KP_EXECUTING;

        setupKPThread = Components.classes["@mozilla.org/thread-manager;1"].getService().newThread(0);
                   
        setupKPThread.dispatch(new mainWindow.KFexecutableInstallerRunner(threadFixer,"/silent","IC1KPSetupFinished", mainWindow, window), setupKPThread.DISPATCH_NORMAL);
      // var temp = new mainWindow.KFexecutableInstallerRunner("KeePass-2.06-Beta-Setup.exe","","IC1KPSetupFinished", mainWindow, window);
      // temp.run();
        
    } else if (installState & KF_INSTALL_STATE_KP_DOWNLOADING)
    {
        hideSection('IC1setupNETdownloaded');
        showSection('IC1setupKPdownloading');
    }
} catch (err) {
            Components.utils.reportError(err);
        }    
}

function IC1setupNET(mainWindow) {
try {
    if ((installState & KF_INSTALL_STATE_NET_DOWNLOADED) && (installState & KF_INSTALL_STATE_SELECTED_PRI))
    {
        var checkTest = mainWindow.KFMD5checksumVerification(KF_NET_FILE_NAME,KF_NET_FILE_CHECKSUM);
        
        if (checkTest)
            mainWindow.KFLog.info("File checksum succeeded.");
        else
            mainWindow.KFLog.error("File checksum failed. Download corrupted?");
        // TODO: kick user back to start if it fails
        
        hideSection('IC1setupNETdownloading');
        showSection('IC1setupNETdownloaded');
        
        // start the pre-download of the KeePass setup file (while user installs .Net...)
        installState |= KF_INSTALL_STATE_KP_DOWNLOADING;
        mainWindow.KFdownloadFile("IC1PriDownload", KF_KP_DOWNLOAD_PATH + KF_KP_FILE_NAME, KF_KP_FILE_NAME, mainWindow, window);
        
        installState |= KF_INSTALL_STATE_NET_EXECUTING;

        var file = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        file.append(KF_NET_FILE_NAME);
        
        threadFixer = file.path;
        
        setupNETThread = Components.classes["@mozilla.org/thread-manager;1"].getService().newThread(0);
                   
        setupNETThread.dispatch(new mainWindow.KFexecutableInstallerRunner(threadFixer,"","IC1NETSetupFinished", mainWindow, window), setupNETThread.DISPATCH_NORMAL);
        
    }
} catch (err) {
            Components.utils.reportError(err);
        }
}


function IC1setupNET35(mainWindow) {
try {
    if ((installState & KF_INSTALL_STATE_NET35_DOWNLOADED) && (installState & KF_INSTALL_STATE_SELECTED_SEC))
    {
        var checkTest = mainWindow.KFMD5checksumVerification(KF_NET35_FILE_NAME,KF_NET35_FILE_CHECKSUM);
        
        if (checkTest)
            mainWindow.KFLog.info("File checksum succeeded.");
        else
            mainWindow.KFLog.error("File checksum failed. Download corrupted?");
        // TODO: kick user back to start if it fails
        
        hideSection('IC1setupNET35downloading');
        showSection('IC1setupNET35downloaded');
        
        // start the pre-download of the KeePass setup file (while user installs .Net...)
        installState |= KF_INSTALL_STATE_KP_DOWNLOADING;
        mainWindow.KFdownloadFile("IC1SecDownload", KF_KP_DOWNLOAD_PATH + KF_KP_FILE_NAME, KF_KP_FILE_NAME, mainWindow, window);
        
        installState |= KF_INSTALL_STATE_NET_EXECUTING;

        var file = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        file.append(KF_NET35_FILE_NAME);
        
        threadFixer = file.path;
        
        setupNETThread = Components.classes["@mozilla.org/thread-manager;1"].getService().newThread(0);
                   
        setupNETThread.dispatch(new mainWindow.KFexecutableInstallerRunner(threadFixer,"","IC1NET35SetupFinished", mainWindow, window), setupNETThread.DISPATCH_NORMAL);
        
    }
} catch (err) {
            Components.utils.reportError(err);
        }
}
/********************
END:
functions to support the execution of Install Case 1
********************/



/********************
START:
functions to support the execution of Install Case 2
********************/
function IC2finished(mainWindow) {

    hideSection('IC2KIdownloaded');
    showSection('InstallFinished');
    
    launchAndConnectToKeePass();
}

function IC2setupKI(mainWindow) {
try {
    if ((installState & KF_INSTALL_STATE_KI_DOWNLOADED) && (installState & KF_INSTALL_STATE_KP_EXECUTED))
    {
        // we don't checksum bundled DLLs since if they are compromised the whole XPI, including this file, could be too
        hideSection('IC2setupKPdownloaded');
        showSection('IC2KIdownloaded');
        
        installState |= KF_INSTALL_STATE_KI_EXECUTING;

        // we've just run the official KeePass installer so can be pretty confident that we can automatically find the installation directory (though if we get time, we should handle exceptions too...)
        var keePassLocation;
        var KeePassEXEfound;
        KeePassEXEfound = false;
        keePassLocation = "not installed";

        keePassLocation = mainWindow.keeFoxInst._discoverKeePassInstallLocation();
        if (keePassLocation != "not installed")
        {
            KeePassEXEfound = mainWindow.keeFoxInst._confirmKeePassInstallLocation(keePassLocation);
        }
        
        if (KeePassEXEfound)
        {
            copyKeeICEFilesTo(keePassLocation);

            installState ^= KF_INSTALL_STATE_KI_EXECUTING; // we can safely assume this bit is already set since no other threads are involved
            installState |= KF_INSTALL_STATE_KI_EXECUTED;
            IC2finished(mainWindow);
        } else
        {
            hideSection('IC2KIdownloaded');
            showSection('ERRORInstallButtonMain');
        }
    }
    } catch (err) {
            Components.utils.reportError(err);
        }
}

function IC2setupKP(mainWindow) {
try {

    if ((installState & KF_INSTALL_STATE_KP_DOWNLOADED) && (installState & KF_INSTALL_STATE_SELECTED_PRI))
    {
        var checkTest = mainWindow.KFMD5checksumVerification(KF_KP_FILE_NAME,KF_KP_FILE_CHECKSUM);
        
        if (checkTest)
            mainWindow.KFLog.info("File checksum succeeded.");
        else
            mainWindow.KFLog.error("File checksum failed. Download corrupted?");
        // TODO: kick user back to start if it fails
        
        hideSection('IC2setupKPdownloading'); // if applicable
        showSection('IC2setupKPdownloaded');

        installState |= KF_INSTALL_STATE_KP_EXECUTING;

        var file = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        file.append(KF_KP_FILE_NAME);
        
        threadFixer = file.path;
        
        setupKPThread = Components.classes["@mozilla.org/thread-manager;1"].getService().newThread(0);
                   
        setupKPThread.dispatch(new mainWindow.KFexecutableInstallerRunner(threadFixer,"/silent","IC2KPSetupFinished", mainWindow, window), setupKPThread.DISPATCH_NORMAL);
      // var temp = new mainWindow.KFexecutableInstallerRunner("KeePass-2.06-Beta-Setup.exe","","IC1KPSetupFinished", mainWindow, window);
      // temp.run();
        
    } else if (installState & KF_INSTALL_STATE_KP_DOWNLOADING)
    {
        showSection('IC2setupKPdownloading');
    }
} catch (err) {
            Components.utils.reportError(err);
        }    
}

function IC2setupCustomKP(mainWindow) {
try {

    if ((installState & KF_INSTALL_STATE_KP_DOWNLOADED) && (installState & KF_INSTALL_STATE_SELECTED_SEC))
    {
        var checkTest = mainWindow.KFMD5checksumVerification(KF_KP_FILE_NAME,KF_KP_FILE_CHECKSUM);
        
        if (checkTest)
            mainWindow.KFLog.info("File checksum succeeded.");
        else
            mainWindow.KFLog.error("File checksum failed. Download corrupted?");
        // TODO: kick user back to start if it fails
        
        hideSection('IC2setupKPdownloading'); // if applicable (probably this was never shown)
        showSection('IC2setupKPdownloaded');

        installState |= KF_INSTALL_STATE_KP_EXECUTING;

        var file = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        file.append(KF_KP_FILE_NAME);
        
        threadFixer = file.path;
        
        setupKPThread = Components.classes["@mozilla.org/thread-manager;1"].getService().newThread(0);
                   
        setupKPThread.dispatch(new mainWindow.KFexecutableInstallerRunner(threadFixer,"","IC2KPSetupFinished", mainWindow, window), setupKPThread.DISPATCH_NORMAL);
      // var temp = new mainWindow.KFexecutableInstallerRunner("KeePass-2.06-Beta-Setup.exe","","IC1KPSetupFinished", mainWindow, window);
      // temp.run();
        
    } else if (installState & KF_INSTALL_STATE_KP_DOWNLOADING)
    {
        showSection('IC2setupKPdownloading');
    }
} catch (err) {
            Components.utils.reportError(err);
        }    
}
/********************
END:
functions to support the execution of Install Case 2
********************/




/********************
START:
functions to support the execution of IC5 (and IC2 tertiary)
********************/
/*
 IC5 (and IC2 tertiary)
 test to see if KeePass is installed in specified location, if it isn't, extract the portable zip file there
 then in either case we will copy the KeeICE files into the plugin folder
 */
 //TODO: rearrange this install process so that the folder is chosen first (before download - or during?) just in case user cancels first (so minimal bandwidth wasted)
function IC5zipKP() {

    if ((installState & KF_INSTALL_STATE_KPZIP_DOWNLOADED) && (installState & KF_INSTALL_STATE_SELECTED_TER))
    {
        var checkTest = mainWindow.KFMD5checksumVerification(KF_KPZIP_FILE_NAME, KF_KPZIP_FILE_CHECKSUM);
        
        if (checkTest)
            mainWindow.KFLog.info("File checksum succeeded.");
        else
            mainWindow.KFLog.error("File checksum failed. Download corrupted?");
        // TODO: kick user back to start if it fails
        
        hideSection('IC5zipKPdownloading');
        showSection('IC5installing');

        installState |= KF_INSTALL_STATE_KPZIP_EXECUTING;

        const nsIFilePicker = Components.interfaces.nsIFilePicker;

        var fp = Components.classes["@mozilla.org/filepicker;1"]
                       .createInstance(nsIFilePicker);
        fp.init(window, "Choose a directory for KeePass", nsIFilePicker.modeGetFolder);
        //fp.appendFilters(nsIFilePicker.filterAll);

        var rv = fp.show();
        if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
            var folder = fp.file;
            // Get the path as string. Note that you usually won't 
            // need to work with the string paths.
            var path = fp.file.path;
          
            //TODO: is KeePass already there?
            
            //TODO: permissions, failures, missing directories, etc. etc.
            extractKPZip (KF_KPZIP_FILE_NAME, folder);
              
            var KeePassEXEfound;
            
            mainWindow.keeFoxInst._keeFoxExtension.prefs.setValue("keePassInstalledLocation",path+"\\"); //TODO: probably should store the file object itself rather than string version (X-Plat)
            KeePassEXEfound = mainWindow.keeFoxInst._confirmKeePassInstallLocation(path+"\\");
            mainWindow.KFLog.info("KeePass install location set to: " + path+"\\");
            if (!KeePassEXEfound)
            {
                mainWindow.keeFoxInst._keeFoxExtension.prefs.setValue("keePassInstalledLocation","");
            } else
            {
                copyKeeICEFilesTo(path);
            }
                
            hideSection('IC5installing');
            showSection('InstallFinished');
            
            launchAndConnectToKeePass();

        } else
        {
            hideSection('installProgressView');
            showSection('installationStartView');
            hideSection('IC5installing');
        }
        
        
    } else if (installState & KF_INSTALL_STATE_KPZIP_DOWNLOADING)
    {
        showSection('IC5zipKPdownloading');
    }

    
  
}
/********************
END:
functions to support the execution of IC5 (and IC2 tertiary)
********************/


/********************
START:
functions initiated by user choice in UI
********************/
/*
 IC1 and IC4
 run (with admin rights escalation) the .NET 2 installer 
 */
function setupExeInstall() {

    hideInstallView();
    showProgressView();
    
    if (installState & KF_INSTALL_STATE_NET_DOWNLOADING)
    {
        
        showSection('IC1setupNETdownloading');
    }
 
    // call this now and let it decide if it is the right time to run the installer or wait until it's called again by progress listener
    installState |= KF_INSTALL_STATE_SELECTED_PRI;
    IC1setupNET(mainWindow);
}

/*
 IC2
 run (quietly and with admin rights escalation) the keepass 2.x exe installer
 and copy the KeeICE files into place
 */
function KPsetupExeSilentInstall() {

    hideInstallView();
    showProgressView();
    
    if (installState & KF_INSTALL_STATE_KP_DOWNLOADING)
    {
        
        showSection('IC2setupKPdownloading');
    }
 
    // call this now and let it decide if it is the right time to run the installer or wait until it's called again by progress listener
    installState |= KF_INSTALL_STATE_SELECTED_PRI;
    IC2setupKP(mainWindow);
}


/*
 IC3 and IC6
 copy KeeICE files to the known KeePass 2.x location
 TODO: detect access denied failures and prompt user accordingly (this could happen if an admin installed KeePass at an earlier time)
 */
function copyKIToKnownKPLocationInstall() {

    hideInstallView();
    
    keePassLocation = mainWindow.keeFoxInst._keeFoxExtension.prefs.getValue("keePassInstalledLocation", "not installed");

    if (keePassLocation == "not installed")
    {
        Components.utils.reportError("We seem to have forgetton where KeePass is installed!");
        showSection('ERRORInstallButtonMain');
    } else
    {
        showProgressView();
        showSection('IC3installing');
      
        copyKeeICEFilesTo(keePassLocation);
  
        hideSection('IC3installing');
        showSection('InstallFinished');
        
        launchAndConnectToKeePass();

    }
}


/*
 IC5 (and IC2 tertiary)
 test to see if KeePass is installed in specified location, if it isn't, extract the portable zip file there
 then in either case we will copy the KeeICE files into the plugin folder
 */
function copyKPToSpecificLocationInstall() {

//TODO: cancel KPsetup download
    
    // start the download if it hasn't already been done
    if (!(installState & KF_INSTALL_STATE_KPZIP_DOWNLOADING) && !(installState & KF_INSTALL_STATE_KPZIP_DOWNLOADED))
    {
        installState |= KF_INSTALL_STATE_KPZIP_DOWNLOADING | KF_INSTALL_STATE_KI_DOWNLOADED;
        mainWindow.KFdownloadFile("IC5PriDownload", KF_KPZIP_DOWNLOAD_PATH + KF_KPZIP_FILE_NAME, KF_KPZIP_FILE_NAME, mainWindow, window);
    }
    
    hideInstallView();
    showProgressView();
    
    showSection('IC5zipKPdownloading');
 
    // call this now and let it decide if it is the right time to run the installer or wait until it's called again by progress listener
    installState |= KF_INSTALL_STATE_SELECTED_TER; // mini-cheat: this state is also applied even when the true state is IC5 primary but this is the quick way to identify this varient of the installation
    
    IC5zipKP(mainWindow);
}


/*
 IC1 and IC4
 run (with admin rights escalation) the .NET 3.5 installer 
 */
function setupNET35ExeInstall() {

//TODO: cancel net2 download

installState = KF_INSTALL_STATE_NET35_DOWNLOADING | KF_INSTALL_STATE_KI_DOWNLOADED;
            mainWindow.KFdownloadFile("IC1SecDownload", KF_NET35_DOWNLOAD_PATH + KF_NET35_FILE_NAME, KF_NET35_FILE_NAME, mainWindow, window);
    hideInstallView();
    showProgressView();
    
    showSection('IC1setupNET35downloading');

 
    // call this now and let it decide if it is the right time to run the installer or wait until it's called again by progress listener
    installState |= KF_INSTALL_STATE_SELECTED_SEC;
    
    // we just call IC1 straight away since we currently treat IC1 and IC4 as the same situation
    IC1setupNET35(mainWindow);
}


/*
 (IC2 secondary and IC5 secondary)
 run (with admin rights escalation) the keepass 2.x exe installer
 and copy the KeeICE files into place
 */
function KPsetupExeInstall() {

    hideInstallView();
    showProgressView();
    
    if (installState & KF_INSTALL_STATE_KP_DOWNLOADING)
    {
        
        showSection('IC2setupKPdownloading');
    }
 
    // call this now and let it decide if it is the right time to run the installer or wait until it's called again by progress listener
    installState |= KF_INSTALL_STATE_SELECTED_SEC;
    IC2setupCustomKP(mainWindow);
}

/********************
END:
functions initiated by user choice in UI
********************/



/********************
START:
utility functions
********************/
function launchAndConnectToKeePass()
{
    // Tell KeeFox that KeeICE has been installed so it will reguarly
    // attempt to connect to KeePass when the timer goes off.
    var Application = Components.classes["@mozilla.org/fuel/application;1"].getService(Components.interfaces.fuelIApplication);
    var keeFoxExtension = Application.extensions.get('chris.tomlinson@keefox');
    var keeFoxStorage = keeFoxExtension.storage;

    keeFoxStorage.set("KeeICEInstalled", true)
    
    // launch KeePass and then try to connect to KeeICE
    // (after 7.5 seconds although it might work sooner than that)
    // If it's still not ready by then, a 10 second repeat timer
    // in the main KF.js file will be activated
    mainWindow.keeFoxInst.launchKeePass("-welcomeToKeeFox");
    var event = { notify: function(timer) { mainWindow.keeFoxInst.startICEcallbackConnector(); } }
    
    var timer = Components.classes["@mozilla.org/timer;1"].createInstance(Components.interfaces.nsITimer);
    timer.initWithCallback(event, 7500, Components.interfaces.nsITimer.TYPE_ONE_SHOT);
    mainWindow.KFLog.info("Installation timer started");
}

function userHasAdminRights(mainWindow) {

    var isAdmin;
    isAdmin = false;

    isAdmin = mainWindow.keeFoxInst.IsUserAdministrator();
    if (isAdmin)
    {
        mainWindow.KFLog.info("User has administrative rights");
        return true;
    } else
    {
        mainWindow.KFLog.info("User does not have administrative rights");
        return false;
    }
}

function checkDotNetFramework(mainWindow) {
    var dotNetFrameworkFound;
    dotNetFrameworkFound = false;
      
    // platform is a string with one of the following values: "Win32", "Linux i686", "MacPPC", "MacIntel", or other.
    if (window.navigator.platform == "Win32") {
        var wrk = Components.classes["@mozilla.org/windows-registry-key;1"].createInstance(Components.interfaces.nsIWindowsRegKey);
        wrk.open(wrk.ROOT_KEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft",
        wrk.ACCESS_READ);
        if (wrk.hasChild(".NETFramework")) {
            var subkey = wrk.openChild(".NETFramework", wrk.ACCESS_READ);
            if (subkey.hasChild("policy")) {
                var subkey2 = subkey.openChild("policy", subkey.ACCESS_READ);
                if (subkey2.hasChild("v2.0")) {
                    var subkey3 = subkey2.openChild("v2.0", subkey2.ACCESS_READ);
                    if (subkey3.hasValue("50727")) {
                        if (subkey3.readStringValue("50727") == "50727-50727") {
                            dotNetFrameworkFound = true;
                            mainWindow.KFLog.info(".NET framework has been found");
                        }
                    }
                    subkey3.close();
                }
                subkey2.close();
            }
            subkey.close();
        }
        wrk.close();
    }
    return dotNetFrameworkFound;
}

//TODO: doesn't work on Windows 7
function copyKeeICEFilesTo(keePassLocation)
{
    var destFolder = Components.classes["@mozilla.org/file/local;1"]
    .createInstance(Components.interfaces.nsILocalFile);
    destFolder.initWithPath(keePassLocation);
    destFolder.append("plugins");

    try {
        if (!destFolder.exists())
            destFolder.create(destFolder.DIRECTORY_TYPE, 0775);
        var KeeICEfile = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        KeeICEfile.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        KeeICEfile.append("KeeICE.dll");
        KeeICEfile.copyTo(destFolder,"");

        var ICEfile = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        ICEfile.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        ICEfile.append("Ice.dll");
        ICEfile.copyTo(destFolder,"");
    } catch (ex)
    {
        Components.utils.reportError(ex);
        //TODO: check it really was the assumed 0x80520008 (NS_ERROR_FILE_ALREADY_EXISTS) exception
    }

    var KeeICEDLLfound;
    
    var keeICELocation;
    keeICELocation = "not installed";

    keeICELocation = mainWindow.keeFoxInst._discoverKeeICEInstallLocation(); // this also stores the preference
    KeeICEDLLfound = mainWindow.keeFoxInst._confirmKeeICEInstallLocation(keeICELocation);
    
    // if we can't find the DLLs, they were probably not copied because of a permissions fault so lets try
    // a fully escalated executable to get them put into place
    if (!KeeICEDLLfound)
    {
        mainWindow.keeFoxInst._keeFoxExtension.prefs.setValue("keeICEInstalledLocation","");
        runKeeICEExecutableInstaller(keePassLocation);
        
        keeICELocation = "not installed";

        keeICELocation = mainWindow.keeFoxInst._discoverKeeICEInstallLocation(); // this also stores the preference
        KeeICEDLLfound = mainWindow.keeFoxInst._confirmKeeICEInstallLocation(keeICELocation);
        
        // still not found!
        if (!KeeICEDLLfound)
        {
            mainWindow.keeFoxInst._keeFoxExtension.prefs.setValue("keeICEInstalledLocation","");
            //TODO: better handle this unusual situation (e.g. Vista user cancelling UAC request)
        }
    }
}

function runKeeICEExecutableInstaller(keePassLocation)
{
    var destFolder = Components.classes["@mozilla.org/file/local;1"]
    .createInstance(Components.interfaces.nsILocalFile);
    destFolder.initWithPath(keePassLocation);
    destFolder.append("plugins");
    
    var file = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(mainWindow.keeFoxInst._myDepsDir());
        file.append(KF_KI_FILE_NAME);

    mainWindow.keeFoxInst._KeeFoxXPCOMobj.RunAnInstaller(file.path,'"' + mainWindow.keeFoxInst._myDepsDir() + '" "' + destFolder.path + '"');

}

// TODO: would be nice if this could go in a seperate thread but my guess is that would be masochistic
// in the mean time I've tried sticking some thread.processNextEvent calls in at strategic points...
function extractKPZip (zipFilePath, storeLocation) {

    var zipFile = Components.classes["@mozilla.org/file/local;1"]
        .createInstance(Components.interfaces.nsILocalFile);
        zipFile.initWithPath(mainWindow.keeFoxInst._myDepsDir());

        zipFile.append(zipFilePath);
        
    var thread = Components.classes["@mozilla.org/thread-manager;1"]
                        .getService(Components.interfaces.nsIThreadManager)
                        .currentThread;

    var zipReader = Components.classes["@mozilla.org/libjar/zip-reader;1"]
                     .createInstance(Components.interfaces.nsIZipReader);
    zipReader.open(zipFile);
    //zipReader.test(null);
    
    // create directories first
    var entries = zipReader.findEntries("*/");
    while (entries.hasMore()) {
        thread.processNextEvent(true);
        var entryName = entries.getNext();
        var target = getItemFile(entryName);
        if (!target.exists()) {
            try {
                target.create(Components.interfaces.nsILocalFile.DIRECTORY_TYPE, 0744);
            }
            catch (e) {
                alert("extractKPZip: failed to create target directory for extraction " +
                " file = " + target.path + ", exception = " + e + "\n");
            }
        }
    }

    entries = zipReader.findEntries(null);
    while (entries.hasMore()) {
        var entryName = entries.getNext();
        target = getItemFile(entryName);
        if (target.exists())
            continue;

        thread.processNextEvent(true);

        try {
            target.create(Components.interfaces.nsILocalFile.NORMAL_FILE_TYPE, 0744); //TODO: different permissions for special files on linux. e.g. 755 for main executable? not sure how it works with Mono though so needs much more reading...
        }
        catch (e) {
            alert("extractKPZip: failed to create target file for extraction " +
            " file = " + target.path + ", exception = " + e + "\n");
        }
        zipReader.extract(entryName, target);
    }
    zipReader.close();
    

    function getItemFile( filePath) {
        var itemLocation = storeLocation.clone();
        var parts = filePath.split("/");
        for (var i = 0; i < parts.length; ++i)
          itemLocation.append(parts[i]);
        return itemLocation;
    }

}

/********************
END:
utility functions
********************/
  
/*

TODO in 0.7: 
see if moving nsfile stuff outside of seperate thread stops all crashes again or if i just been unlucky
test each install start point in VMs
scrolling box for chrome window
work out and configure sensible keepass defaults for use as a mainly browser based password management system (put option changes into the new user wizzard in KeeICE)

*/