// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "FriendsAndChatPrivatePCH.h"

/**
 * Implements the FriendsAndChat module.
 */
class FFriendsAndChatModule
	: public IFriendsAndChatModule
{
public:

	// IModuleInterface interface

	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}

	virtual void ShutdownStyle() override
	{
		FFriendsAndChatModuleStyle::Shutdown();
	}

};


IMPLEMENT_MODULE( FFriendsAndChatModule, FriendsAndChat );
