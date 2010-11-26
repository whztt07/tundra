// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "EC_Script.h"
#include "IScriptInstance.h"

#include "IAttribute.h"
#include "IAssetTransfer.h"
#include "Entity.h"
#include "LoggingFunctions.h"
#include "AssetRefListener.h"

DEFINE_POCO_LOGGING_FUNCTIONS("EC_Script")

EC_Script::~EC_Script()
{
    SAFE_DELETE(scriptInstance_);
}

void EC_Script::SetScriptInstance(IScriptInstance *instance)
{
    // If we already have a script instance, unload and delete it.
    if (scriptInstance_)
    {
        scriptInstance_->Unload();
        SAFE_DELETE(scriptInstance_);
    }

    scriptInstance_ = instance;
}

void EC_Script::Run(const QString &name)
{
    if (!scriptInstance_)
    {
        LogError("Cannot perform Run(), no script instance set");
        return;
    }

    if (name.isEmpty() || (name == scriptRef.Get().ref))
        scriptInstance_->Run();
}

void EC_Script::Unload(const QString &name)
{
    if (!scriptInstance_)
    {
        LogError("Cannot perform Unload(), no script instance set");
        return;
    }

    if (name.isEmpty() || (name == scriptRef.Get().ref))
        scriptInstance_->Unload();
}

EC_Script::EC_Script(IModule *module):
    IComponent(module->GetFramework()),
    scriptRef(this, "Script ref"),
    type(this, "Type"),
    runOnLoad(this, "Run on load", false),
    scriptInstance_(0)
{
    connect(this, SIGNAL(OnAttributeChanged(IAttribute*, AttributeChange::Type)),
        SLOT(HandleAttributeChanged(IAttribute*, AttributeChange::Type)));
    connect(this, SIGNAL(ParentEntitySet()), SLOT(RegisterActions()));

    scriptAsset = boost::shared_ptr<AssetRefListener>(new AssetRefListener);
    connect(scriptAsset.get(), SIGNAL(Downloaded(IAssetTransfer*)), this, SLOT(ScriptAssetLoaded(IAssetTransfer*)));
}

void EC_Script::HandleAttributeChanged(IAttribute* attribute, AttributeChange::Type change)
{
    if (attribute == &scriptRef)
    {
        scriptAsset->HandleAssetRefChange(attribute);
        /*
        if (scriptRef.Get().ref != lastRef_)
        {
            emit ScriptRefChanged(scriptRef.Get().ref);
            lastRef_ = scriptRef.Get().ref;
        }
        */
    }
}

void EC_Script::ScriptAssetLoaded(IAssetTransfer *transfer)
{
    emit ScriptAssetChanged(transfer->asset);
}

void EC_Script::RegisterActions()
{
    Scene::Entity *entity = GetParentEntity();
    assert(entity);
    if (entity)
    {
        entity->ConnectAction("RunScript", this, SLOT(Run(const QString &)));
        entity->ConnectAction("UnloadScript", this, SLOT(Unload(const QString &)));
    }
}

