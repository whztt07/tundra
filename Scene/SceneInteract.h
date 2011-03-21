/**
 *  For conditions of distribution and use, see copyright notice in license.txt
 *
 *  @file   SceneInteract.h
 *  @brief  Transforms generic mouse and keyboard input events to
*           input-related Entity Action for scene entities.
 */

#include "ForwardDefines.h"
#include "SceneFwd.h"
#include "InputFwd.h"

#include <QObject>

class RaycastResult;

#ifndef incl_Scene_SceneInteract_h
#define incl_Scene_SceneInteract_h

/// Transforms generic mouse and keyboard input events to input-related Entity Action for scene entities.
/** Peforms raycast each frame and if entity is hit, performs the following actions on it:
    -"MouseHoverIn"
    -"MouseHover"
    -"MouseHoverOut"
    -"MousePress"
*/
class SceneInteract : public QObject
{
    Q_OBJECT

public:
    //! Costructor. Framework takes ownership of this object
    /// \param fw Framework.
    SceneInteract();

    //! Destructor.
    ~SceneInteract() {}

    //! Set framework.
    void Initialize(Foundation::Framework *framework);

signals:
    //! Emitted when scene was clicked and raycast hit an entity.
    /// \param entity Hit entity.
    void EntityClicked(Scene::Entity *entity);

    //! Emitted when scene was clicked and raycast hit an entity.
    /// \param entity Hit entity.
    void EntityClicked(Scene::Entity *entity, Qt::MouseButton button);

    //! Emitted when scene was clicked and raycast hit an entity.
    /// \param entity Hit entity.
    void EntityClicked(Scene::Entity *entity, Qt::MouseButton button, RaycastResult *raycastResult);

private:
    //! Performs raycast to last known mouse cursor position.
    RaycastResult* Raycast();

    Foundation::Framework *framework_; ///< Framework.
    InputContextPtr input_; ///< Input context.
    Foundation::RendererWeakPtr renderer_; ///< Renderer pointer.
    int lastX_; ///< Last known mouse cursor's x position.
    int lastY_; ///< Last known mouse cursor's y position.
    bool itemUnderMouse_; ///< Was there widget under mouse in last known position.
    Scene::EntityWeakPtr lastHitEntity_; ///< Last entity raycast has hit.

private slots:
    //! Executes "MouseHover" action each frame is raycast has hit and entity.
    void Update();

    //! Handles key events from input service.
    /// \param e Key event.
    void HandleKeyEvent(KeyEvent *e);

    //! Handles mouse events from input service.
    /// \param e Mouse event.
    void HandleMouseEvent(MouseEvent *e);
};

#endif