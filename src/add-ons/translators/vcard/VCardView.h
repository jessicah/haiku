#ifndef VCARD_VIEW_H
#define VCARD_VIEW_H

#include <View.h>

#include "TranslatorSettings.h"

class VCardView : public BView {
	public:
		VCardView(const BRect& frame, const char* name, uint32 resizeMode,
			uint32 flags, TranslatorSettings* settings);
		virtual ~VCardView();

	private:
		TranslatorSettings *fSettings;
};

#endif // #ifndef VCARD_VIEW_H
