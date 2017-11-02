#include "dollycam.h"
#include "bakkesmod\wrappers\gamewrapper.h"
#include "bakkesmod\wrappers\replaywrapper.h"
#include "bakkesmod\wrappers\camerawrapper.h"

DollyCam::DollyCam(std::shared_ptr<GameWrapper> _gameWrapper, std::shared_ptr<CVarManagerWrapper> _cvarManager, std::shared_ptr<IGameApplier> _gameApplier)
{
	currentPath = std::unique_ptr<savetype>(new savetype());
	gameWrapper = _gameWrapper;
	cvarManager = _cvarManager;
	gameApplier = _gameApplier;
}

DollyCam::~DollyCam()
{
}

bool DollyCam::TakeSnapshot()
{
	if (!gameWrapper->IsInReplay())
		return false;

	ReplayWrapper sw = gameWrapper->GetGameEventAsReplay();
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (sw.IsNull())
		return false;

	CameraSnapshot save;
	save.timeStamp = sw.GetReplayTimeElapsed();
	save.FOV = flyCam.GetFOV();
	save.location = flyCam.GetLocation();
	save.rotation = CustomRotator(flyCam.GetRotation());
	save.frame = sw.GetCurrentReplayFrame();
	
	currentPath->insert(std::make_pair(save.timeStamp, save));
}

bool DollyCam::IsActive()
{
	return isActive;
}

void DollyCam::Activate()
{
	if (isActive)
		return;

	isActive = true;
	CVarWrapper interpMode = cvarManager->getCvar("dolly_interpmode");

	switch (interpMode.getIntValue())
	{
	case 0:
		interpStrategy = make_shared<LinearInterpStrategy>(LinearInterpStrategy(currentPath));
		break;
	default:
		cvarManager->log("Cannot activate dollycam, unrecognized interp mode");
		Deactivate();
		return;
		break;
	}
	cvarManager->log("Dollycam activated");
}

void DollyCam::Deactivate()
{
	if (!isActive)
		return;
	CameraWrapper flyCam = gameWrapper->GetCamera();
	if (!flyCam.GetCameraAsActor().IsNull()) {
		flyCam.SetLockedFOV(false);
	}
	isActive = false;
	cvarManager->log("Dollycam deactivated");
}

void DollyCam::Apply()
{
	ReplayWrapper sw = gameWrapper->GetGameEventAsReplay();
	CameraWrapper flyCam = gameWrapper->GetCamera();
	NewPOV pov = interpStrategy->GetPOV(sw.GetReplayTimeElapsed(), sw.GetCurrentReplayFrame());
	if (pov.FOV < 1) //Invalid camerastate
		return;
	flyCam.SetPOV(pov.ToPOV());
}