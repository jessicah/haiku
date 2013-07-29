#include <Application.h>

#include "PersonTranslator.h"

int main()
{
	BApplication app("application/x-vnd.Haiku-PersonTranslator");
	status_t result;
	result = LaunchTranslatorWindow(new PersonTranslator,
		"VCard Settings", BRect(0, 0, 225, 175));
	if (result == B_OK) {
		app.Run();
		return 0;
	} else
		return 1;
}

