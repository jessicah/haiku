/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef RATE_PACKAGE_WINDOW_H
#define RATE_PACKAGE_WINDOW_H

#include <Window.h>

#include "PackageInfo.h"
#include "TextDocument.h"


class RatePackageWindow : public BWindow {
public:
								RatePackageWindow(BWindow* parent, BRect frame,
									const BString& preferredLanguage,
									const StringList& supportedLanguages);
	virtual						~RatePackageWindow();

	virtual	void				MessageReceived(BMessage* message);

			void				SetPackage(const PackageInfoRef& package);

private:
			void				_SendRating();

private:
			TextDocumentRef		fRatingText;
			float				fRating;
			BString				fStability;
			BString				fCommentLanguage;
			PackageInfoRef		fPackage;
};


#endif // RATE_PACKAGE_WINDOW_H