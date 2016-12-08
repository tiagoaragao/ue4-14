﻿// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using AutomationTool;
using UnrealBuildTool;

public class CopyUsingDistillFileSet : BuildCommand
{
	public override void ExecuteBuild()
	{
		FileReference Project = new FileReference(ParseParamValue("ProjectPath"));
		string ManifestFile = ParseParamValue("ManifestFile");
		string UE4Exe = ParseParamValue("UE4Exe");
		string[] Maps = ParseParamValue("Maps").Split(new char[] { '+', ';' }, StringSplitOptions.RemoveEmptyEntries).ToArray();
		string Parameters = ParseParamValue("Parameters");
		DirectoryReference FromDir = new DirectoryReference(ParseParamValue("FromDir"));
		DirectoryReference ToDir = new DirectoryReference(ParseParamValue("ToDir"));

		// Run commandlet to get files required for maps
		List<string> SourceFiles = GenerateDistillFileSetsCommandlet(Project, ManifestFile, UE4Exe, Maps, Parameters);

		// Convert Source file paths to output paths and copy
		IEnumerable<FileReference> TargetFiles = SourceFiles.Select(x => FileReference.Combine(ToDir, new FileReference(x).MakeRelativeTo(FromDir)));
		CommandUtils.ThreadedCopyFiles(SourceFiles, TargetFiles.Select(x => x.FullName).ToList());
	}
};
