// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"

#include "RendererEvents.h"
#include "InputModuleOIS.h"
#include "BufferedKeyboard.h"
#include "Mapper.h"

namespace Input
{
    const char *DeviceType[6] = {"OISUnknown", "OISKeyboard", "OISMouse", "OISJoyStick",
							 "OISTablet", "OISOther"};

    InputModuleOIS::InputModuleOIS() : 
        ModuleInterfaceImpl(type_static_)
        , input_manager_(0)
        , keyboard_(0)
        , mouse_(0)
        , joy_(0)
        , event_category_(0)
    {       
    }

    InputModuleOIS::~InputModuleOIS()
    {
    }

    // virtual
    void InputModuleOIS::Load()
    {
        LogInfo("Module " + Name() + " loaded.");
    }

    // virtual
    void InputModuleOIS::Unload()
    {
        LogInfo("Module " + Name() + " unloaded.");
    }

    // virtual
    void InputModuleOIS::Initialize()
    {
        LogInfo("*** Initializing OIS ***");

        if (framework_->GetServiceManager()->IsRegistered(Foundation::Service::ST_Renderer) == false)
        {
            LogError("Failed to initialize. No renderer service registered.");
            return;
        }
        Foundation::RenderServiceInterface *renderer = framework_->GetService<Foundation::RenderServiceInterface>(Foundation::Service::ST_Renderer);
        size_t window_handle = renderer->GetWindowHandle();
        if (window_handle == 0)
        {
            LogError("Failed to initialize. No open window.");
            return;
        }
        
        event_category_ = framework_->GetEventManager()->RegisterEventCategory("Input");

        OIS::ParamList pl;
        pl.insert(std::make_pair(std::string("WINDOW"), Core::ToString(window_handle)));

#if defined OIS_WIN32_PLATFORM
//        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_BACKGROUND" )));
        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_FOREGROUND" )));
        pl.insert(std::make_pair(std::string("w32_mouse"), std::string("DISCL_NONEXCLUSIVE")));
#elif defined OIS_LINUX_PLATFORM
        pl.insert(std::make_pair(std::string("x11_mouse_grab"), std::string("false")));
        pl.insert(std::make_pair(std::string("x11_mouse_hide"), std::string("false")));
#endif

        // throws exception if fails, not handled since it should be rather fatal
        input_manager_ = OIS::InputManager::createInputSystem( pl );

        buffered_keyboard_ = BufferedKeyboardPtr(new BufferedKeyboard(framework_));
        keyboard_ = static_cast<OIS::Keyboard*>(input_manager_->createInputObject( OIS::OISKeyboard, false ));      
        LogInfo("Keyboard input initialized.");
   
        mouse_ = static_cast<OIS::Mouse*>(input_manager_->createInputObject( OIS::OISMouse, false ));
        LogInfo("Mouse input initialized.");
        try
        {
            joy_ = static_cast<OIS::JoyStick*>(input_manager_->createInputObject( OIS::OISJoyStick, false ));
            LogInfo("Joystick / gamepad input initialized.");
        }
        catch(...)
        {
            LogInfo("Joystick / gamepad not found.");
        }

        // set window default width / height. Should be updated when window size changes
        WindowResized(renderer->GetWindowWidth(), renderer->GetWindowHeight());

        unsigned int v = input_manager_->getVersionNumber();

        std::stringstream ss;
	    ss << "OIS Version: " << (v>>16 ) << "." << ((v>>8) & 0x000000FF) << "." << (v & 0x000000FF);
        LogInfo(ss.str());

		LogInfo("Release Name: " + input_manager_->getVersionName());
		LogInfo("Manager: " + input_manager_->inputSystemName());
        LogInfo("Total Keyboards: " + Core::ToString(input_manager_->getNumberOfDevices(OIS::OISKeyboard)));
		LogInfo("Total Mice: " + Core::ToString(input_manager_->getNumberOfDevices(OIS::OISMouse)));
		LogInfo("Total JoySticks: " + Core::ToString(input_manager_->getNumberOfDevices(OIS::OISJoyStick)));
        

	    //List all devices
        OIS::DeviceList list = input_manager_->listFreeDevices();
        for( OIS::DeviceList::iterator i = list.begin(); i != list.end(); ++i )
            LogInfo("\tDevice: " + std::string(DeviceType[i->first]) + " Vendor: " + i->second);

        key_mapping_ = MapperPtr(new Mapper(this));


        LogInfo("Module " + Name() + " initialized.");
    }

    // virtual 
    void InputModuleOIS::Uninitialize()
    {
        WindowClosed();

        key_mapping_.reset();

        LogInfo("Module " + Name() + " uninitialized.");
    }

    // virtual 
    void InputModuleOIS::Update(Core::f64 frametime)
    {
        if( keyboard_ && mouse_ )
        {
            keyboard_->capture();
	        mouse_->capture();
    	    if ( joy_ ) joy_->capture();

            const OIS::MouseState &ms = mouse_->getMouseState();
            if (ms.Z.rel != 0)
            {
                Events::SingleAxisMovement mw;
                mw.z_.rel_ = ms.Z.rel;
                mw.z_.abs_ = ms.Z.abs;
                framework_->GetEventManager()->SendEvent(event_category_, Events::SCROLL, &mw);
            }

            // we assume GetMouseMovement() will be called several times in one frame
            // It makes sense then to only query OIS once for the mouse state and then
            // return the same state per frame.
            movement_.x_.rel_ = ms.X.rel;
            movement_.x_.abs_ = ms.X.abs;

            movement_.y_.rel_ = ms.Y.rel;
            movement_.y_.abs_ = ms.Y.abs;

            movement_.z_.rel_ = ms.Z.rel;
            movement_.z_.abs_ = ms.Z.abs;

            
            for (size_t i=0 ; i<listened_keys_.size() ; ++i)
            {
                bool key_released = false;
                if (keyboard_->isKeyDown(listened_keys_[i].key_))
                {
                    if(!listened_keys_[i].pressed_)
                    {
                        // check modifiers in a bit convoluted way. All combos of ctrl+a, ctrl+alt+a and ctrl+alt+shift+a must work!
                        if (((listened_keys_[i].modifier_ & OIS::Keyboard::Alt)   == 0 || keyboard_->isModifierDown(OIS::Keyboard::Alt))  &&
                            ((listened_keys_[i].modifier_ & OIS::Keyboard::Ctrl)  == 0 || keyboard_->isModifierDown(OIS::Keyboard::Ctrl)) &&
                            ((listened_keys_[i].modifier_ & OIS::Keyboard::Shift) == 0 || keyboard_->isModifierDown(OIS::Keyboard::Shift))) 
                        {
                            listened_keys_[i].pressed_ = true;
                            framework_->GetEventManager()->SendEvent(event_category_, listened_keys_[i].pressed_event_id_, NULL);
                        } else
                        {
                            key_released = true;
                        }
                    }
                }
                else if(listened_keys_[i].pressed_)
                {
                    key_released = true;
                }

                if (key_released)
                {
                    listened_keys_[i].pressed_ = false;
                    framework_->GetEventManager()->SendEvent(event_category_, listened_keys_[i].released_event_id_, NULL);
                }
            }
         
        }
        if (buffered_keyboard_)
            buffered_keyboard_->Update();
    }

    // virtual
    bool InputModuleOIS::HandleEvent(Core::event_category_id_t category_id, Core::event_id_t event_id, Foundation::EventDataInterface* data)
    {
        bool handled = false;

        if (framework_->GetEventManager()->QueryEventCategory("Renderer") == category_id)
        {
            if (event_id == OgreRenderer::Event::WINDOW_CLOSED)
                WindowClosed();

            if (event_id == OgreRenderer::Event::WINDOW_RESIZED)
            {
                WindowResized(  checked_static_cast<OgreRenderer::Event::WindowResized*>(data)->width_,
                                checked_static_cast<OgreRenderer::Event::WindowResized*>(data)->height_ );
            }
        }

        // no need to mark events handled
        return handled;
    }

    void InputModuleOIS::WindowClosed()
    {
        if( input_manager_ )
        {
            input_manager_->destroyInputObject( mouse_ );
            input_manager_->destroyInputObject( keyboard_ );
            input_manager_->destroyInputObject( joy_ );

            OIS::InputManager::destroyInputSystem(input_manager_);

            input_manager_ = 0;
            mouse_ = 0;
            keyboard_ = 0;
            joy_ = 0;

            buffered_keyboard_.reset();
        }
    }

    void InputModuleOIS::WindowResized(int width, int height)
    {
        if (mouse_)
        {
            const OIS::MouseState &ms = mouse_->getMouseState();
            ms.width = width;
            ms.height = height;
        }
    }

    
    void InputModuleOIS::RegisterUnbufferedKeyEvent(OIS::KeyCode key, Core::event_id_t pressed_event, Core::event_id_t released_event, int modifier)
    {
        UnBufferedKeyEventInfo keyeventinfo;
        
        keyeventinfo.pressed_ = false;
        keyeventinfo.pressed_event_id_ = pressed_event;
        keyeventinfo.released_event_id_ = released_event;
        keyeventinfo.key_ = key;
        keyeventinfo.modifier_ = modifier;
        listened_keys_.push_back(keyeventinfo);        
    }    

}

