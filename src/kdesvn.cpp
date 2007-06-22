/***************************************************************************
 *   Copyright (C) 2005-2007 by Rajko Albrecht                             *
 *   ral@alwins-world.de                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/


#include "kdesvn.h"
#include "urldlg.h"

#include <qdragobject.h>
#include <kprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qcursor.h>

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kdeversion.h>
#include <kstatusbar.h>
#include <kaccel.h>
#include <kio/netaccess.h>
#include <kfiledialog.h>
#include <kconfig.h>
#include <kurl.h>
#include <kurldrag.h>
#include <kurlrequesterdlg.h>
#include <khelpmenu.h>
#include <kmenubar.h>
#include <kpopupmenu.h>
#include <kactionclasses.h>
#include <kmessagebox.h>
#include <kstdaccel.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kstandarddirs.h>
#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>
#include <kdebug.h>
#include <klibloader.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kdirselectdialog.h>

#ifdef TESTING_RC
#include <kcrash.h>
#endif

kdesvn::kdesvn()
    : KParts::MainWindow( 0, "kdesvn" ),
      KBookmarkOwner()
{
    m_part = 0;
#ifdef TESTING_RC
    setXMLFile(TESTING_RC);
    kdDebug()<<"Using test rc file in " << TESTING_RC << endl;
    // I hate this crashhandler in development
    KCrash::setCrashHandler(0);
#else
    setXMLFile("kdesvnui.rc");
#endif
    setStandardToolBarMenuEnabled(true);
    // then, setup our actions
    setupActions();
    // and a status bar
    statusBar()->show();

    m_bookmarkFile = locateLocal("appdata",QString::fromLatin1("bookmarks.xml"),true);

    m_BookmarkManager = KBookmarkManager::managerForFile(m_bookmarkFile,false);
    m_BookmarkManager->setShowNSBookmarks(false);
    m_BookmarkManager->setEditorOptions(QString::fromLatin1("KDE Svn"),false);

    m_BookmarksActionmenu = new KActionMenu(i18n("&Bookmarks"),"bookmark",actionCollection(),"bookmarks");
    m_BookmarksActionmenu->setDelayed(false);
    m_BookmarksActionmenu->setEnabled(true);

    m_Bookmarkactions = new KActionCollection( this );
    m_Bookmarkactions->setHighlightingEnabled( true );
    connectActionCollection( m_Bookmarkactions );

    m_pBookmarkMenu = new KBookmarkMenu(m_BookmarkManager,this,m_BookmarksActionmenu->popupMenu(),m_Bookmarkactions,true);
//    m_BookmarksActionmenu->plug(menuBar());
    // this routine will find and load our Part.  it finds the Part by
    // name which is a bad idea usually.. but it's alright in this
    // case since our Part is made for this Shell
    KLibFactory *factory = KLibLoader::self()->factory("libkdesvnpart");

    if (factory)
    {
        kdDebug()<<"Name: " << factory->className()<<endl;
        // now that the Part is loaded, we cast it to a Part to get
        // our hands on it
        m_part = static_cast<KParts::ReadOnlyPart *>(factory->create(this,
                                "kdesvn_part", "KParts::ReadOnlyPart" ));

        if (m_part)
        {
            // tell the KParts::MainWindow that this is indeed the main widget
            setCentralWidget(m_part->widget());

            KAction*tmpAction;
            tmpAction = new KAction(i18n("Create and open new repository"),"filenew",
                KShortcut(),m_part->widget(),SLOT(slotCreateRepo()),actionCollection(),"subversion_create_repo");
            tmpAction->setToolTip(i18n("Create and opens a new local subversion repository"));
            tmpAction = new KAction(i18n("Dump repository to file"),"filenew",
                KShortcut(),m_part->widget(),SLOT(slotDumpRepo()),actionCollection(),"subversion_dump_repo");
            tmpAction->setToolTip(i18n("Dump a subversion repository to a file"));
            tmpAction = new KAction(i18n("Hotcopy a repository"),"filenew",
                KShortcut(),m_part->widget(),SLOT(slotHotcopy()),actionCollection(),"subversion_hotcopy_repo");
            tmpAction->setToolTip(i18n("Hotcopy a subversion repository to a new folder"));
            tmpAction = new KAction(i18n("Load dump into repository"),"filenew",
                KShortcut(),m_part->widget(),SLOT(slotLoaddump()),actionCollection(),"subversion_load_repo");
            tmpAction->setToolTip(i18n("Load a dump file into a repository."));
            tmpAction = new KAction(i18n("Add ssh identities to ssh-agent"),"password",
                KShortcut(),m_part,SLOT(slotSshAdd()),actionCollection(),"kdesvn_ssh_add");
            tmpAction->setToolTip(i18n("Force add ssh-identities to ssh-agent for future use."));
            tmpAction = new KAction(i18n("Info about kdesvn part"), "kdesvn",
                        KShortcut(), m_part, SLOT(showAboutApplication()), actionCollection(), "help_about_kdesvnpart");
            tmpAction->setToolTip(i18n("Shows info about the kdesvn plugin not the standalone app."));

            /* enable tooltips in statusbar for menu */
            actionCollection()->setHighlightingEnabled(true);
            connectActionCollection(actionCollection());
            // and integrate the part's GUI with the shells
            createGUI(m_part);
            connectActionCollection(m_part->actionCollection());
        } else {
            KMessageBox::error(this, i18n("Could not load the part:\n")+KLibLoader::self()->lastErrorMessage());
            kapp->quit();
            return;
        }
    }
    else
    {
        // if we couldn't find our Part, we exit since the Shell by
        // itself can't do anything useful
        KMessageBox::error(this, i18n("Could not find our part")+QString(":\n")+KLibLoader::self()->lastErrorMessage());
        kapp->quit();
        // we return here, cause kapp->quit() only means "exit the
        // next time we enter the event loop...
        return;
    }
    setAutoSaveSettings();
}

void kdesvn::connectActionCollection( KActionCollection *coll )
{
    if (!coll)return;
    connect( coll, SIGNAL( actionStatusText( const QString & ) ),
             this, SLOT( changeStatusbar( const QString & ) ) );
    connect( coll, SIGNAL( clearStatusText() ),
             this, SLOT( resetStatusBar() ) );
}

void kdesvn::disconnectActionCollection( KActionCollection *coll )
{
    if (!coll)return;
}

kdesvn::~kdesvn()
{
}

void kdesvn::load(const KURL& url)
{
    if (m_part) {
        bool ret = m_part->openURL(url);
        KAction * ac=actionCollection()->action("file_open_recent");
        KRecentFilesAction*rac = 0;
        if (ac) {
            rac = (KRecentFilesAction*)ac;
        }
        if (!ret) {
            changeStatusbar(i18n("Could not open url %1").arg(url.prettyURL()));
            if (rac) {
                rac->removeURL(url);
            }
        } else {
            resetStatusBar();
            if (rac) {
                rac->addURL(url);
            }
        }
        if (rac) {
            rac->saveEntries(KGlobal::config(),"recent_files");
        }
    }
}

void kdesvn::setupActions()
{
    KAction*ac;
    KStdAction::open(this, SLOT(fileOpen()), actionCollection());
    KStdAction::openNew(this,SLOT(fileNew()),actionCollection());
    ac = KStdAction::close(this,SLOT(fileClose()),actionCollection());
    ac->setEnabled(getMemberList()->count()>1);
    KStdAction::quit(kapp, SLOT(quit()), actionCollection());

    KRecentFilesAction*rac = KStdAction::openRecent(this,SLOT(load(const KURL&)),actionCollection());
    if (rac)
    {
        rac->setMaxItems(8);
        rac->loadEntries(KGlobal::config(),"recent_files");
    }

    KStdAction::keyBindings(this, SLOT(optionsConfigureKeys()), actionCollection());
    KStdAction::configureToolbars(this, SLOT(optionsConfigureToolbars()), actionCollection());

    m_statusbarAction = KStdAction::showStatusbar(this, SLOT(optionsShowStatusbar()), actionCollection());

    KToggleAction *toggletemp;
    toggletemp = new KToggleAction(i18n("Load last opened URL on start"),KShortcut(),
            actionCollection(),"toggle_load_last_url");
    toggletemp->setToolTip(i18n("Reload last opened url if no one is given on commandline"));
    KConfigGroup cs(KGlobal::config(),"startup");
    toggletemp->setChecked(cs.readBoolEntry("load_last_on_start",false));
    connect(toggletemp,SIGNAL(toggled(bool)),this,SLOT(slotLoadLast(bool)));
}

void kdesvn::optionsShowStatusbar()
{
    // this is all very cut and paste code for showing/hiding the
    // statusbar
    if (m_statusbarAction->isChecked())
        statusBar()->show();
    else
        statusBar()->hide();
}

void kdesvn::fileClose()
{
    if (m_part) m_part->closeURL();
    if (getMemberList()->count()>1) {
        close();
    } else {
        enableClose(false);
    }
}

void kdesvn::saveProperties(KConfig *config)
{
    // the 'config' object points to the session managed
    // config file.  anything you write here will be available
    // later when this app is restored
    if (!m_part) return;
    if (!m_part->url().isEmpty()) {
#if KDE_IS_VERSION(3,1,3)
        config->writePathEntry("lastURL", m_part->url().prettyURL());
#else
        config->writeEntry("lastURL", m_part->url().prettyURL());
#endif
    }
}

void kdesvn::readProperties(KConfig *config)
{
    // the 'config' object points to the session managed
    // config file.  this function is automatically called whenever
    // the app is being restored.  read in here whatever you wrote
    // in 'saveProperties'

    QString url = config->readPathEntry("lastURL");

    if (!url.isEmpty() && m_part)
        m_part->openURL(KURL(url));
}

void kdesvn::fileNew()
{
    // this slot is called whenever the File->New menu is selected,
    // the New shortcut is pressed (usually CTRL+N) or the New toolbar
    // button is clicked

    // create a new window
    (new kdesvn)->show();
    enableClose(true);
}

void kdesvn::fileOpen()
{
    KURL url = UrlDlg::getURL(this);
    if (!url.isEmpty())
        load(url);
}

void kdesvn::changeStatusbar(const QString& text)
{
    // display the text on the statusbar
    statusBar()->message(text);
}

void kdesvn::resetStatusBar()
{
    statusBar()->message(i18n("Ready"));
}

void kdesvn::openBookmarkURL (const QString &_url)
{
    if (!_url.isEmpty() && m_part)
        load(_url);
}

QString kdesvn::currentURL () const
{
    if (!m_part) return "";
    return m_part->url().prettyURL();
}

void kdesvn::enableClose(bool how)
{
    KAction * ac;
    if ( (ac=actionCollection()->action("file_close"))) {
        ac->setEnabled(how);
    }
}

/*!
    \fn kdesvn::slotUrlOpened(bool)
 */
void kdesvn::slotUrlOpened(bool how)
{
    enableClose(how);
}


/*!
    \fn kdesvn::optionsConfigureToolbars()
 */
void kdesvn::optionsConfigureToolbars()
{
#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,0)
    saveMainWindowSettings(KGlobal::config(), autoSaveGroup());
# else
    saveMainWindowSettings(KGlobal::config() );
# endif
#else
    saveMainWindowSettings(KGlobal::config() );
#endif

    // use the standard toolbar editor
    KEditToolbar dlg(factory());
    connect(&dlg, SIGNAL(newToolbarConfig()),
            this, SLOT(applyNewToolbarConfig()));
    dlg.exec();
}


/*!
    \fn kdesvn::applyNewToolbarConfig()
 */
void kdesvn::applyNewToolbarConfig()
{
#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,0)
    applyMainWindowSettings(KGlobal::config(), autoSaveGroup());
# else
    applyMainWindowSettings(kdesvnPart::config());
# endif
#else
    applyMainWindowSettings(kdesvnPart::config());
#endif
}

void kdesvn::optionsConfigureKeys()
{
    KKeyDialog kdlg(true,m_part->widget());
    kdlg.insert(actionCollection());
    kdlg.insert(m_part->actionCollection());
    bool b = kdlg.configure(true);
    if (b) {
        kdlg.commitChanges();
    }
}


/*!
    \fn kdesvn::queryExit()
 */
bool kdesvn::queryExit()
{
    if (m_part) {
        KConfigGroup cs(KGlobal::config(),"startup");
#if KDE_IS_VERSION(3,1,3)
        cs.writePathEntry("lastURL", m_part->url().prettyURL());
#else
        cs.writeEntry("lastURL", m_part->url().prettyURL());
#endif
    }
    return KParts::MainWindow::queryExit();
}


/*!
    \fn kdesvn::checkReload()
 */
void kdesvn::checkReload()
{
    KConfigGroup cs(KGlobal::config(),"startup");
    if (!cs.readBoolEntry("load_last_on_start",false))
        return;

    QString url = cs.readPathEntry("lastURL");

    if (!url.isEmpty() && m_part)
        load(KURL(url));
}


/*!
    \fn kdesvn::slotLoadLast(bool)
 */
void kdesvn::slotLoadLast(bool how)
{
    KConfigGroup cs(KGlobal::config(),"startup");
    cs.writeEntry("load_last_on_start",how);
}

#include "kdesvn.moc"
