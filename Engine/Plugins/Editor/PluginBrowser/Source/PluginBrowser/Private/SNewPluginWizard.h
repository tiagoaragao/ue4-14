// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SlateFwd.h"
#include "Input/Reply.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "IPluginWizardDefinition.h"

class ITableRow;
class SFilePathBlock;
class STableViewBase;
enum class ECheckBoxState : uint8;

DECLARE_LOG_CATEGORY_EXTERN(LogPluginWizard, Log, All);

class SFilePathBlock;

/**
 * A wizard to create a new plugin
 */
class SNewPluginWizard : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SNewPluginWizard){}
	SLATE_END_ARGS()

	/** Constructor */
	SNewPluginWizard();

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs, TSharedPtr<SDockTab> InOwnerTab, TSharedPtr<IPluginWizardDefinition> InPluginWizardDefinition = nullptr);

private:
	/**
	 * Called when Folder Path textbox changes value
	 * @param InText The new Plugin Folder Path text
	 */
	void OnFolderPathTextChanged(const FText& InText);

	/**
	 * Called to generate a widget for the specified list item
	 * @param Item The template information for this row
	 * @param OwnerTable The table that owns these rows
	 * @return The widget for this template
	 */
	TSharedRef<ITableRow> OnGenerateTemplateRow(TSharedRef<FPluginTemplateDescription> InItem, const TSharedRef<STableViewBase>& OwnerTable);

	/**
	 * Called when the template selection changes
	 */
	void OnTemplateSelectionChanged(TSharedPtr<FPluginTemplateDescription> InItem, ESelectInfo::Type SelectInfo);

	/**
	 * Called when Plugin Name textbox changes value
	 * @param InText The new Plugin name text
	 */
	void OnPluginNameTextChanged(const FText& InText);

	/** Handler for when the Browse button is clicked */
	FReply OnBrowseButtonClicked();

	/**
	 * Validates both the current path and plugin name as the final step in name
	 * validation requires a valid path.
	 */
	void ValidateFullPluginPath();

	/**
	 * Whether we are currently able to create a plugin
	 */
	bool CanCreatePlugin() const;

	/**
	 * Get the path where we will create a plugin
	 */
	FText GetPluginDestinationPath() const;

	/** Get the current name of the plugin */
	FText GetCurrentPluginName() const;

	/** Get the full path of the .plugin file we will create */
	FString GetPluginFilenameWithPath() const;

	/**
	 * Whether we will create a plugin in the engine directory
	 */
	ECheckBoxState IsEnginePlugin() const;

	/**
	 * Called when state of Engine plugin checkbox changes
	 * @param NewCheckedState New state of the checkbox
	 */
	void OnEnginePluginCheckboxChanged(ECheckBoxState NewCheckedState);

	/**
	 * This is where all the magic happens.
	 * Create actual plugin using parameters collected from other widgets
	 */
	FReply OnCreatePluginClicked();

	/**
	 * Copies a file and adds to a list of created files
	 * @param DestinationFile Where the file will be copied
	 * @param SourceFile Original file to copy
	 * @param InOutCreatedFiles Array of created files to add to
	 * @return Whether the file was copied successfully
	 */
	bool CopyFile(const FString& DestinationFile, const FString& SourceFile, TArray<FString>& InOutCreatedFiles);

	/**
	 * Writes a plugin descriptor file to disk
	 * @param PluginModuleName Name of the plugin and its module
	 * @param UPluginFilePath Path where the descriptor file should be written
	 * @return Whether the files was written successfully
	 */
	bool WritePluginDescriptor(const FString& PluginModuleName, const FString& UPluginFilePath, bool bCanContainContent, bool bHasModules);

	/**
	 * Displays an editor pop up error notification
	 * @param ErrorMessage Message to display in the pop up
	 */
	void PopErrorNotification(const FText& ErrorMessage);

	/**
	 * Helper function to delete given directory
	 * @param InPath - full path to directory that you want to remove
	 */
	void DeletePluginDirectory(const FString& InPath);

private:
	/** The current plugin wizard definition */
	TSharedPtr<IPluginWizardDefinition> PluginWizardDefinition;

	/** The list view for template selection */
	TSharedPtr<SListView<TSharedRef<FPluginTemplateDescription>>> ListView;

	/** Absolute path to game plugins directory so we don't have to convert it repeatedly */
	FString AbsoluteGamePluginPath;

	/** Absolute path to engine plugins directory so we don't have to convert it repeatedly */
	FString AbsoluteEnginePluginPath;

	/** Last Path used to browse, so that we know it will open dialog */
	FString LastBrowsePath;

	/** Path where you want to create the plugin*/
	FString PluginFolderPath;

	/** Name of the plugin you want to create*/
	FText PluginNameText;

	/** File Path widget that user will choose plugin location and name with */
	TSharedPtr<SFilePathBlock> FilePathBlock;

	/** Whether the path of the plugin entered is currently valid */
	bool bIsPluginPathValid;

	/** Whether the name of the plugin entered is currently valid */
	bool bIsPluginNameValid;

	/** Whether we want to create a plugin in the engine folder */
	bool bIsEnginePlugin;

	/** Tab that owns this wizard so that we can ask to close after completion */
	TWeakPtr<SDockTab> OwnerTab;
};
