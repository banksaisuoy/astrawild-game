using UnrealBuildTool;
using System.Collections.Generic;

public class ASTRAWILDEditorTarget : TargetRules
{
    public ASTRAWILDEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        bOverrideBuildEnvironment = true;
        ExtraModuleNames.Add("AstrawildCore");
    }
}
