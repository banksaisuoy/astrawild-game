#include "AstrawildGameMode.h"

#include "AstrawildPlayerCharacter.h"

AAstrawildGameMode::AAstrawildGameMode()
{
    DefaultPawnClass = AAstrawildPlayerCharacter::StaticClass();
}
