#include "Modules/ModuleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void StartHomeworkVerification();

class FHomeworkEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("HW1Verify"))) StartHomeworkVerification();
	}
};

IMPLEMENT_MODULE(FHomeworkEditorModule, HW1Editor)
