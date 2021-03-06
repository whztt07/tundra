print("Loading EC_SoundListener example script.");

var touchable_comp = 0;
if (me.Component("Touchable"))
{
	touchable_comp = me.GetComponent("Touchable");
	AddConnections();
}
else
	scene.ComponentAdded.connect(OnComponentAdded);

function OnObjectClicked()
{
	var sound_listener = me.GetComponent("SoundListener");
	sound_listener.active = true;
	sound_listener.OnChanged();
}

function OnComponentAdded(entity, component)
{
	if (me.id != entity.id && component.typeName != "EC_Touchable")
		return;
		
	touchable_comp = component;
	AddConnections();
}

function AddConnections()
{
	touchable_comp.MousePressed.connect(OnObjectClicked);
}