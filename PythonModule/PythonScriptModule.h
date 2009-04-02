// For conditions of distribution and use, see copyright notice in license.txt

#ifndef incl_PythonScriptingModule_h
#define incl_PythonScriptingModule_h

#include "Foundation.h"
#include "StableHeaders.h"
#include "ModuleInterface.h"
#include "ComponentRegistrarInterface.h"
#include "ServiceManager.h"

//testing receiving events, now from the net module 'cause nothing else sends yet
#include "OpenSimProtocolModule.h" //XXX for login hack
#include "NetInMessage.h"
#include "NetMessage.h"

#include <Python/Python.h>
//#include "Script.h"

namespace Foundation
{
    class Framework;
}

namespace PythonScript
{
    //! A scripting module using Python
    class MODULE_API PythonScriptModule : public Foundation::ModuleInterfaceImpl
    {
    public:
        PythonScriptModule();
        virtual ~PythonScriptModule();

		//the module interface
        virtual void Load();
        virtual void Unload();
        virtual void Initialize();
        virtual void PostInitialize();
        virtual void Uninitialize();
        virtual void Update(Core::f64 frametime);

		//handling events
        virtual bool HandleEvent(
            Core::event_category_id_t category_id,
            Core::event_id_t event_id, 
            Foundation::EventDataInterface* data);

		//! callback for console command
        Console::CommandResult ConsoleRunString(const Core::StringVector &params);
        Console::CommandResult ConsoleRunFile(const Core::StringVector &params);
        Console::CommandResult ConsoleReset(const Core::StringVector &params);
        
        MODULE_LOGGING_FUNCTIONS

        //! returns name of this module. Needed for logging.
        static const std::string &NameStatic() { return Foundation::Module::NameFromType(type_static_); }
        static const Foundation::Module::Type type_static_ = Foundation::Module::MT_PythonScript;

		//for api test XXX
		void SendChatFromViewerPacket(const char *msg);

		Foundation::Framework *GetFramework() { return framework_; }
		/* API calls exposed to py. should be private but testing now here
		   will probably be wrapping the actual modules in separate files,
		   but first test now here. also will use boostpy or something, but now first by hand */
		PyObject* sendChat(PyObject *self, PyObject *args);


	private:
		
		//basic feats
		void PythonScriptModule::RunString(const char* codestr);
		void PythonScriptModule::RunFile(const std::string &modulename);
		void PythonScriptModule::Reset();

		// Category id for incoming messages.
		Core::event_category_id_t inboundCategoryID_;

        // for the hack that login directly from here XXX Pointer to the network interface.
		OpenSimProtocol::OpenSimProtocolModule *netInterface_;
		ClientParameters myInfo_;

		//bool bRunning_;

		// first stab at having a py defined event handler
		// now just one - eventually could have a list/dict of these?
		PyObject *pName, *pModule, *pDict, *pFunc;
	    PyObject *pArgs, *pValue;

    /* python interpreter? 
        OgreRenderer::RendererPtr renderer_;*/

		PyMethodDef EmbMethods[];
	};
}

#endif
