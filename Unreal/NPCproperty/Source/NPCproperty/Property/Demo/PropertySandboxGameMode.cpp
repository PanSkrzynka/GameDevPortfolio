#include "Property/Demo/PropertySandboxGameMode.h"
#include "Property/Demo/PropertySandboxHUD.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"

APropertySandboxGameMode::APropertySandboxGameMode()
{
	static ConstructorHelpers::FClassFinder<APawn> Player(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	static ConstructorHelpers::FClassFinder<APlayerController> Controller(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonPlayerController"));
	if (Player.Succeeded()) DefaultPawnClass = Player.Class;
	if (Controller.Succeeded()) PlayerControllerClass = Controller.Class;
	HUDClass = APropertySandboxHUD::StaticClass();
}
