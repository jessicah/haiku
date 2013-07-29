#ifndef PERSON_VIEW_H
#define PERSON_VIEW_H

#include <View.h>

#include "TranslatorSettings.h"

class PersonView : public BView {
	public:
		PersonView(const BRect& frame, const char* name, uint32 resizeMode,
			uint32 flags, TranslatorSettings* settings);
		virtual ~PersonView();

	private:
		TranslatorSettings *fSettings;
};

#endif // #ifndef Person_VIEW_H
