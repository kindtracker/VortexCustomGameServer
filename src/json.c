#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "vcgs.h"

static void WriteJsonString(FILE *File, const char *Value) {
  fputc('"', File);

  if (Value) {
    for (const unsigned char *Character = (const unsigned char *)Value;
         *Character; Character++) {
      switch (*Character) {
      case '"':
        fputs("\\\"", File);
        break;
      case '\\':
        fputs("\\\\", File);
        break;
      case '\b':
        fputs("\\b", File);
        break;
      case '\f':
        fputs("\\f", File);
        break;
      case '\n':
        fputs("\\n", File);
        break;
      case '\r':
        fputs("\\r", File);
        break;
      case '\t':
        fputs("\\t", File);
        break;
      default:
        if (*Character < 0x20) {
          fprintf(File, "\\u%04x", *Character);
        } else {
          fputc(*Character, File);
        }
        break;
      }
    }
  }

  fputc('"', File);
}

static void WriteJsonFloatArray(FILE *File, const FloatArray *Array) {
  fputc('[', File);

  for (size_t Index = 0; Index < Array->Count; Index++) {
    if (Index > 0)
      fputc(',', File);

    fprintf(File, "%.9g", Array->Values[Index]);
  }

  fputc(']', File);
}

static void WriteJsonBool(FILE *File, bool Value) {
  fputs(Value ? "true" : "false", File);
}

static void WriteJsonOptionalId(FILE *File, const OptionalId *Value) {
  if (Value->HasValue)
    fprintf(File, "%llu", (unsigned long long)Value->Value);
  else
    fputs("null", File);
}

static const char *ClassName(ClassId Id) {
  switch (Id) {
  case ClassWorkspace:
    return "Workspace";
  case ClassLighting:
    return "Lighting";
  case ClassPart:
    return "Part";
  case ClassGroup:
    return "Group";
  case ClassFolder:
    return "Folder";
  case ClassIntValue:
    return "IntValue";
  case ClassStringValue:
    return "StringValue";
  case ClassLocalScript:
    return "LocalScript";
  case ClassScript:
    return "Script";
  case ClassModuleScript:
    return "ModuleScript";
  case ClassReplicatedStorage:
    return "ReplicatedStorage";
  case ClassStarterPlayerScripts:
    return "StarterPlayerScripts";
  case ClassServerScriptService:
    return "ServerScriptService";
  case ClassRemoteEvent:
    return "RemoteEvent";
  case ClassBindableEvent:
    return "BindableEvent";
  case ClassRemoteFunction:
    return "RemoteFunction";
  case ClassBindableFunction:
    return "BindableFunction";
  default:
    return "Unknown";
  }
}

static const char *MaterialName(MaterialId Id) {
  switch (Id) {
  case MaterialSmooth:
    return "Smooth";
  case MaterialPlastic:
    return "Plastic";
  case MaterialWood:
    return "Wood";
  case MaterialMetal:
    return "Metal";
  case MaterialGrass:
    return "Grass";
  case MaterialIce:
    return "Ice";
  case MaterialPaint:
    return "Paint";
  default:
    return "Unknown";
  }
}

static void WriteJsonTexture(FILE *File, const Texture *TextureValue) {
  fprintf(File, "{\"Face\":%d,\"Kind\":%u}", (int)TextureValue->Face,
          TextureValue->Kind);
}

static void WriteJsonLight(FILE *File, const Light *LightValue) {
  fputs("{", File);

  fputs("\"Color\":", File);
  WriteJsonFloatArray(File, &LightValue->Color);

  fprintf(File, ",\"Intensity\":%.9g", LightValue->Intensity);
  fprintf(File, ",\"Range\":%.9g", LightValue->Range);
  fprintf(File, ",\"Fov\":%.9g", LightValue->Fov);
  fprintf(File, ",\"Face\":%d", (int)LightValue->Face);

  fputs(",\"IsSpot\":", File);
  WriteJsonBool(File, LightValue->IsSpot);

  fputs("}", File);
}

static void WriteJsonPart(FILE *File, const PartBody *Part) {
  fputs("{", File);

  fputs("\"ParentId\":", File);
  WriteJsonOptionalId(File, &Part->ParentId);

  fputs(",\"DisplayName\":", File);
  if (Part->HasDisplayName)
    WriteJsonString(File, Part->DisplayName);
  else
    fputs("null", File);

  fputs(",\"Position\":", File);
  WriteJsonFloatArray(File, &Part->Position);

  fputs(",\"Rotation\":", File);
  WriteJsonFloatArray(File, &Part->Rotation);

  fputs(",\"Size\":", File);
  WriteJsonFloatArray(File, &Part->Size);

  fputs(",\"Color\":", File);
  WriteJsonFloatArray(File, &Part->Color);

  fputs(",\"Material\":", File);
  WriteJsonString(File, MaterialName(Part->Material));

  fputs(",\"CastShadow\":", File);
  WriteJsonBool(File, Part->CastShadow);

  fputs(",\"Anchored\":", File);
  WriteJsonBool(File, Part->Anchored);

  fputs(",\"CanCollide\":", File);
  WriteJsonBool(File, Part->CanCollide);

  fputs(",\"SpawnLocation\":", File);
  WriteJsonBool(File, Part->SpawnLocation);

  fputs(",\"Locked\":", File);
  WriteJsonBool(File, Part->Locked);

  fputs(",\"CustomAppearance\":", File);
  WriteJsonBool(File, Part->CustomAppearance);

  fputs(",\"Truss\":", File);
  WriteJsonBool(File, Part->Truss);

  fputs(",\"Textures\":[", File);

  for (size_t Index = 0; Index < Part->TextureCount; Index++) {
    if (Index > 0)
      fputc(',', File);

    WriteJsonTexture(File, &Part->Textures[Index]);
  }

  fputs("]", File);

  fputs(",\"PointLight\":", File);
  if (Part->PointLight)
    WriteJsonLight(File, Part->PointLight);
  else
    fputs("null", File);

  fputs(",\"SpotLight\":", File);
  if (Part->SpotLight)
    WriteJsonLight(File, Part->SpotLight);
  else
    fputs("null", File);

  fputs(",\"Collapsed\":", File);
  WriteJsonBool(File, Part->Collapsed);

  fputs("}", File);
}

static void WriteJsonScript(FILE *File, const ScriptBody *Script) {
  fputs("{", File);

  fputs("\"ParentId\":", File);
  WriteJsonOptionalId(File, &Script->ParentId);

  fputs(",\"Source\":", File);

  if (Script->HasSource)
    WriteJsonString(File, Script->Source);
  else
    fputs("null", File);

  fputs(",\"Collapsed\":", File);
  WriteJsonBool(File, Script->Collapsed);

  fputs("}", File);
}

static void WriteJsonVariable(FILE *File, const VariableBody *Variable) {
  fputs("{", File);

  fputs("\"ParentId\":", File);
  WriteJsonOptionalId(File, &Variable->ParentId);

  fputs(",\"HasCollapsed\":", File);
  WriteJsonBool(File, Variable->HasCollapsed);

  fputs(",\"Collapsed\":", File);
  WriteJsonBool(File, Variable->Collapsed);

  fputs("}", File);
}

static void WriteJsonRecord(FILE *File, const Record *RecordValue) {
  fputs("{", File);

  fputs("\"ClassId\":", File);
  fprintf(File, "%d", (int)RecordValue->ClassId);

  fputs(",\"Class\":", File);
  WriteJsonString(File, ClassName(RecordValue->ClassId));

  fputs(",\"Name\":", File);
  WriteJsonString(File, RecordValue->Name);

  fputs(",\"Body\":", File);

  switch (RecordValue->BodyType) {
  case BodyPart:
    WriteJsonPart(File, &RecordValue->Body.Part);
    break;

  case BodyScript:
    WriteJsonScript(File, &RecordValue->Body.Script);
    break;

  case BodyVariable:
    WriteJsonVariable(File, &RecordValue->Body.Variable);
    break;
  }

  fputs("}", File);
}

static void WriteJsonLighting(FILE *File, const Lighting *LightingValue) {
  fputs("{", File);

  fputs("\"AmbientColor\":", File);
  WriteJsonFloatArray(File, &LightingValue->AmbientColor);

  fprintf(File, ",\"Brightness\":%.9g", LightingValue->Brightness);

  fputs(",\"SunColor\":", File);
  WriteJsonFloatArray(File, &LightingValue->SunColor);

  fprintf(File, ",\"SunIlluminance\":%.9g", LightingValue->SunIlluminance);

  fputs(",\"SunShadowMapsEnabled\":", File);
  WriteJsonBool(File, LightingValue->SunShadowMapsEnabled);

  fputs(",\"SunDirection\":", File);
  WriteJsonFloatArray(File, &LightingValue->SunDirection);

  fputs("}", File);
}

int WriteGameJson(const Document *DocumentValue, const char *Path) {
  FILE *File = fopen(Path, "wb");

  if (!File)
    return -1;

  fputs("{\n", File);

  fprintf(File, "  \"Version\": %u,\n", DocumentValue->Version);

  fputs("  \"ProjectId\":", File);

  if (DocumentValue->ProjectId.Data)
    WriteJsonString(File, DocumentValue->ProjectId.Data);
  else
    fputs("null", File);

  fputs(",\n", File);

  fputs("  \"Lighting\":", File);
  WriteJsonLighting(File, &DocumentValue->Lighting);

  fputs(",\n", File);

  fputs("  \"Records\": [\n", File);

  for (size_t Index = 0; Index < DocumentValue->RecordCount; Index++) {
    fputs("    ", File);

    WriteJsonRecord(File, &DocumentValue->Records[Index]);

    if (Index + 1 < DocumentValue->RecordCount)
      fputc(',', File);

    fputc('\n', File);
  }

  fputs("  ]\n", File);
  fputs("}\n", File);

  fclose(File);
  return 0;
}
