
#ifndef incl_CommunicationUIModule_h
#define incl_CommunicationUIModule_h

#pragma once

#include "ConfigureDlg.h"
#include "ModuleInterface.h"
//#include "PythonScriptModule.h"


// *********************************************'
// CommunicationViewModel<--->CommunicationView communication:
// 
// - Actions:
//   - ShowContactList(contact_list)
//   - HideContactList(contact_list)      needed ?
//   - ShowSessionInvitation(invitation)
//   - ShowMessageNotification(contact)
//   - ShowPresenceNotification(contact)
// 
// - data objects:
//   - ContactList
//   - Contact
//   - Session
//   - Participient
//
// *********************************************'


namespace Communication
{
	//
	class MODULE_API CommunicationUIModule : public Foundation::ModuleInterfaceImpl, public IConfigureCallBack
	{
	public:
		CommunicationUIModule(void);
		virtual ~CommunicationUIModule(void);

		void Load();
		void Unload();
		void Initialize();
		void PostInitialize();
		void Uninitialize();

		void Update();

		virtual void Callback(std::string aConfigName, std::map<std::string, Foundation::Comms::SettingsAttribute> attributes);

		MODULE_LOGGING_FUNCTIONS
		//! returns name of this module. Needed for logging.
		static const std::string &NameStatic() { return Foundation::Module::NameFromType(type_static_); }
		static const Foundation::Module::Type type_static_ = Foundation::Module::MT_CommunicationUI;

	private:
		static void testCallback(char*);
		

	private:
		void initializeMainCommWindow();
		void OnAccountMenuSettings();
		void OnAccountMenuSetAccountAndPassword();
		void OnAccountMenuConnect();
		void OnAccountMenuDisconnect();
		Glib::RefPtr<Gnome::Glade::Xml> commUI_XML;

		// Widgets
		Gtk::Window *wndCommMain;
		Gtk::Window *dlgAccount;
		Gtk::ActionGroup *actionGroup;
		Foundation::Comms::CommunicationManagerServiceInterface *commManager;
		Foundation::ScriptServiceInterface *scriptService;
		//Foundation::CommunicationUIManagerPtr CommunicationUI_manager_;
		Foundation::ScriptObject* sobj;

		Foundation::ScriptObject* imScriptObject;
	};
}

#endif