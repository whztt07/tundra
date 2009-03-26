// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "EC_ObjIdentity.h"

namespace RexLogic
{
    EC_ObjIdentity::EC_ObjIdentity(Foundation::ModuleInterface* module) : Foundation::ComponentInterface(module->GetFramework())
    {
        RegionHandle = 0;
        Id = 0;
        FullId = ""; // TODO: tucofixme, change type to rexuuid?
        OwnerId = ""; // TODO: tucofixme, change type to rexuuid?
        ParentId = ""; // TODO: tucofixme, change type to rexuuid? 
        
        ObjectName = "";
        Description = "";    
    }

    EC_ObjIdentity::~EC_ObjIdentity()
    {
    }

    void EC_ObjIdentity::HandleObjectUpdate(Foundation::EventDataInterface* data)
    {
        // TODO: tucofixme set values based on data
        RegionHandle = 0;
        Id = 0;
        FullId = ""; 
        OwnerId = "";
        ParentId = "";
        
        ObjectName = "";
        Description = "";
    }
}