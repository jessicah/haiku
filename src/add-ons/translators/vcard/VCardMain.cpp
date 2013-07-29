#include <Application.h>

#include "VCardTranslator.h"

int main()
{
	BApplication app("application/x-vnd.Haiku-VCardTranslator");
	status_t result;
	result = LaunchTranslatorWindow(new VCardTranslator,
		"VCard Settings", BRect(0, 0, 225, 175));
	if (result == B_OK) {
		app.Run();
		return 0;
	} else
		return 1;
}

