#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zstd.h>

#include "vcgs.h"

void Fail(const char *Message) {
  fprintf(stderr, "%s\n", Message);
  abort();
}

void *Allocate(size_t Size) {
  void *Pointer = malloc(Size);

  if (!Pointer && Size != 0)
    Fail("[Vcgs] out of memory");

  return Pointer;
}

void *Reallocate(void *Pointer, size_t Size) {
  void *NewPointer = realloc(Pointer, Size);

  if (!NewPointer && Size != 0)
    Fail("[Vcgs] out of memory");

  return NewPointer;
}

char *DuplicateString(const char *Value) {
  size_t Length = strlen(Value);
  char *Result = Allocate(Length + 1);

  memcpy(Result, Value, Length + 1);

  return Result;
}

Bytes CopyBytes(const uint8_t *Data, size_t Length) {
  Bytes Result = {0};

  if (Length == 0)
    return Result;

  Result.Data = Allocate(Length);
  memcpy(Result.Data, Data, Length);
  Result.Length = Length;

  return Result;
}

FloatArray CreateFloatArray(size_t Count) {
  FloatArray Result = {0};

  if (Count == 0)
    return Result;

  Result.Values = Allocate(sizeof(float) * Count);
  Result.Count = Count;

  return Result;
}

void FreeFloatArray(FloatArray *Array) {
  free(Array->Values);
  Array->Values = NULL;
  Array->Count = 0;
}

void FreeBytes(Bytes *Value) {
  free(Value->Data);
  Value->Data = NULL;
  Value->Length = 0;
}

void BufferReserve(ByteBuffer *Buffer, size_t Additional) {
  size_t Required = Buffer->Length + Additional;

  if (Required <= Buffer->Capacity)
    return;

  size_t Capacity = Buffer->Capacity ? Buffer->Capacity : 256;

  while (Capacity < Required)
    Capacity *= 2;

  Buffer->Data = Reallocate(Buffer->Data, Capacity);
  Buffer->Capacity = Capacity;
}

void BufferWrite(ByteBuffer *Buffer, const void *Data, size_t Length) {
  if (Length == 0)
    return;

  BufferReserve(Buffer, Length);
  memcpy(Buffer->Data + Buffer->Length, Data, Length);
  Buffer->Length += Length;
}

void BufferWriteU8(ByteBuffer *Buffer, uint8_t Value) {
  BufferWrite(Buffer, &Value, 1);
}

void BufferAppend(ByteBuffer *Buffer, const uint8_t *Data, size_t Length) {
  if (Length == 0)
    return;

  if (Buffer->Length + Length > Buffer->Capacity) {
    size_t NewCapacity = Buffer->Capacity == 0 ? 4096 : Buffer->Capacity;

    while (NewCapacity < Buffer->Length + Length)
      NewCapacity *= 2;

    uint8_t *NewData = realloc(Buffer->Data, NewCapacity);

    if (NewData == NULL)
      Fail("[Vcgs] unable to allocate buffer");

    Buffer->Data = NewData;
    Buffer->Capacity = NewCapacity;
  }

  memcpy(Buffer->Data + Buffer->Length, Data, Length);
  Buffer->Length += Length;
}

void BufferFree(ByteBuffer *Buffer) {
  free(Buffer->Data);
  Buffer->Data = NULL;
  Buffer->Length = 0;
  Buffer->Capacity = 0;
}

uint32_t ReadU32Le(const uint8_t *Data) {
  return ((uint32_t)Data[0]) | ((uint32_t)Data[1] << 8) |
         ((uint32_t)Data[2] << 16) | ((uint32_t)Data[3] << 24);
}

int32_t ReadI32Le(const uint8_t *Data) { return (int32_t)ReadU32Le(Data); }

uint64_t ReadU64Le(const uint8_t *Data) {
  return ((uint64_t)Data[0]) | ((uint64_t)Data[1] << 8) |
         ((uint64_t)Data[2] << 16) | ((uint64_t)Data[3] << 24) |
         ((uint64_t)Data[4] << 32) | ((uint64_t)Data[5] << 40) |
         ((uint64_t)Data[6] << 48) | ((uint64_t)Data[7] << 56);
}

float ReadF32Le(const uint8_t *Data) {
  uint32_t Bits = ReadU32Le(Data);
  float Value;

  memcpy(&Value, &Bits, sizeof(Value));

  return Value;
}

void WriteU32Le(uint8_t *Data, uint32_t Value) {
  Data[0] = (uint8_t)(Value);
  Data[1] = (uint8_t)(Value >> 8);
  Data[2] = (uint8_t)(Value >> 16);
  Data[3] = (uint8_t)(Value >> 24);
}

void WriteI32Le(uint8_t *Data, int32_t Value) {
  WriteU32Le(Data, (uint32_t)Value);
}

void WriteU64Le(uint8_t *Data, uint64_t Value) {
  Data[0] = (uint8_t)(Value);
  Data[1] = (uint8_t)(Value >> 8);
  Data[2] = (uint8_t)(Value >> 16);
  Data[3] = (uint8_t)(Value >> 24);
  Data[4] = (uint8_t)(Value >> 32);
  Data[5] = (uint8_t)(Value >> 40);
  Data[6] = (uint8_t)(Value >> 48);
  Data[7] = (uint8_t)(Value >> 56);
}

void WriteF32Le(uint8_t *Data, float Value) {
  uint32_t Bits;

  memcpy(&Bits, &Value, sizeof(Bits));
  WriteU32Le(Data, Bits);
}

static void ReaderRequire(Reader *ReaderValue, size_t Count) {
  if (Count > ReaderValue->Length - ReaderValue->Position)
    Fail("[Vcgs] unexpected end of file");
}

static const uint8_t *ReaderTake(Reader *ReaderValue, size_t Count) {
  ReaderRequire(ReaderValue, Count);

  const uint8_t *Result = ReaderValue->Data + ReaderValue->Position;

  ReaderValue->Position += Count;

  return Result;
}

static uint8_t ReaderU8(Reader *ReaderValue) {
  return *ReaderTake(ReaderValue, 1);
}

static bool ReaderBool(Reader *ReaderValue) {
  return ReaderU8(ReaderValue) != 0;
}

static uint32_t ReaderU32(Reader *ReaderValue) {
  return ReadU32Le(ReaderTake(ReaderValue, 4));
}

static int32_t ReaderI32(Reader *ReaderValue) {
  return ReadI32Le(ReaderTake(ReaderValue, 4));
}

static uint64_t ReaderU64(Reader *ReaderValue) {
  return ReadU64Le(ReaderTake(ReaderValue, 8));
}

static float ReaderF32(Reader *ReaderValue) {
  return ReadF32Le(ReaderTake(ReaderValue, 4));
}

static String ReaderString(Reader *ReaderValue) {
  uint64_t Length = ReaderU64(ReaderValue);

  if (Length > SIZE_MAX - 1)
    Fail("[Vcgs] string too large");

  const uint8_t *Data = ReaderTake(ReaderValue, (size_t)Length);

  String Result;

  Result.Data = Allocate((size_t)Length + 1);
  Result.Length = (size_t)Length;

  memcpy(Result.Data, Data, (size_t)Length);
  Result.Data[Length] = '\0';

  return Result;
}

static OptionalId ReaderOptionalId(Reader *ReaderValue) {
  OptionalId Result = {0};

  Result.HasValue = ReaderBool(ReaderValue);

  if (Result.HasValue)
    Result.Value = ReaderU64(ReaderValue);

  return Result;
}

static void WriterWrite(Writer *WriterValue, const void *Data, size_t Length) {
  BufferWrite(&WriterValue->Data, Data, Length);
}

static void WriterU8(Writer *WriterValue, uint8_t Value) {
  BufferWriteU8(&WriterValue->Data, Value);
}

static void WriterBool(Writer *WriterValue, bool Value) {
  WriterU8(WriterValue, Value ? 1 : 0);
}

static void WriterU32(Writer *WriterValue, uint32_t Value) {
  uint8_t Data[4];

  WriteU32Le(Data, Value);
  WriterWrite(WriterValue, Data, sizeof(Data));
}

static void WriterI32(Writer *WriterValue, int32_t Value) {
  uint8_t Data[4];

  WriteI32Le(Data, Value);
  WriterWrite(WriterValue, Data, sizeof(Data));
}

static void WriterU64(Writer *WriterValue, uint64_t Value) {
  uint8_t Data[8];

  WriteU64Le(Data, Value);
  WriterWrite(WriterValue, Data, sizeof(Data));
}

static void WriterF32(Writer *WriterValue, float Value) {
  uint8_t Data[4];

  WriteF32Le(Data, Value);
  WriterWrite(WriterValue, Data, sizeof(Data));
}

static void WriterString(Writer *WriterValue, const char *Value) {
  size_t Length = strlen(Value);

  WriterU64(WriterValue, Length);
  WriterWrite(WriterValue, Value, Length);
}

static void WriterOptionalId(Writer *WriterValue, OptionalId Value) {
  WriterBool(WriterValue, Value.HasValue);

  if (Value.HasValue)
    WriterU64(WriterValue, Value.Value);
}

static void WriterFree(Writer *WriterValue) { BufferFree(&WriterValue->Data); }

static void ReadVector(Reader *ReaderValue, FloatArray *Result, size_t Count) {
  *Result = CreateFloatArray(Count);

  for (size_t Index = 0; Index < Count; Index++)
    Result->Values[Index] = ReaderF32(ReaderValue);
}

static void WriteVector(Writer *WriterValue, const FloatArray *Value,
                        size_t Count) {
  if (!Value || Value->Count != Count)
    Fail("[Vcgs] invalid vector");

  for (size_t Index = 0; Index < Count; Index++)
    WriterF32(WriterValue, Value->Values[Index]);
}

static const char *GetClassName(ClassId Class) {
  switch (Class) {
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
  }

  return "Unknown";
}

static bool TryGetClassId(const char *Name, ClassId *Result) {
  for (int Index = 0; Index <= 16; Index++) {
    ClassId Class = (ClassId)Index;

    if (strcmp(Name, GetClassName(Class)) == 0) {
      *Result = Class;
      return true;
    }
  }

  return false;
}

static bool IsScriptClass(ClassId Class) {
  return Class == ClassLocalScript || Class == ClassScript ||
         Class == ClassModuleScript;
}

static bool HasNoProperties(ClassId Class) {
  switch (Class) {
  case ClassWorkspace:
  case ClassLighting:
  case ClassGroup:
  case ClassFolder:
  case ClassReplicatedStorage:
  case ClassStarterPlayerScripts:
  case ClassServerScriptService:
  case ClassRemoteEvent:
  case ClassBindableEvent:
  case ClassRemoteFunction:
  case ClassBindableFunction:
    return true;

  default:
    return false;
  }
}

static const char *GetMaterialName(MaterialId Material) {
  switch (Material) {
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
  }

  return "Unknown";
}

static bool TryGetMaterialId(const char *Name, MaterialId *Result) {
  if (strcmp(Name, "Smooth") == 0) {
    *Result = MaterialSmooth;
    return true;
  }

  if (strcmp(Name, "Plastic") == 0) {
    *Result = MaterialPlastic;
    return true;
  }

  if (strcmp(Name, "Wood") == 0) {
    *Result = MaterialWood;
    return true;
  }

  if (strcmp(Name, "Metal") == 0) {
    *Result = MaterialMetal;
    return true;
  }

  if (strcmp(Name, "Grass") == 0) {
    *Result = MaterialGrass;
    return true;
  }

  if (strcmp(Name, "Ice") == 0) {
    *Result = MaterialIce;
    return true;
  }

  if (strcmp(Name, "Paint") == 0) {
    *Result = MaterialPaint;
    return true;
  }

  return false;
}

static const char *GetFaceName(FaceId Face) {
  switch (Face) {
  case FaceRight:
    return "Right";
  case FaceTop:
    return "Top";
  case FaceBack:
    return "Back";
  case FaceLeft:
    return "Left";
  case FaceBottom:
    return "Bottom";
  case FaceFront:
    return "Front";
  }

  return "Unknown";
}

static bool TryGetFaceId(const char *Name, FaceId *Result) {
  if (strcmp(Name, "Right") == 0) {
    *Result = FaceRight;
    return true;
  }

  if (strcmp(Name, "Top") == 0) {
    *Result = FaceTop;
    return true;
  }

  if (strcmp(Name, "Back") == 0) {
    *Result = FaceBack;
    return true;
  }

  if (strcmp(Name, "Left") == 0) {
    *Result = FaceLeft;
    return true;
  }

  if (strcmp(Name, "Bottom") == 0) {
    *Result = FaceBottom;
    return true;
  }

  if (strcmp(Name, "Front") == 0) {
    *Result = FaceFront;
    return true;
  }

  return false;
}

static size_t FindNextHeader(const uint8_t *Data, size_t Length, size_t Start) {
  for (size_t Position = Start; Position + 12 <= Length; Position++) {
    uint32_t Class = ReadU32Le(Data + Position);

    if (Class > 16)
      continue;

    uint64_t NameLength = ReadU64Le(Data + Position + 4);

    if (NameLength == 0 || NameLength > 1000000 ||
        NameLength > Length - Position - 12)
      continue;

    return Position;
  }

  return SIZE_MAX;
}

static Bytes ReadRaw(Reader *ReaderValue, size_t Length) {
  return CopyBytes(ReaderTake(ReaderValue, Length), Length);
}

static Light ReadLight(Reader *ReaderValue, bool IsSpot) {
  Light Result = {0};

  ReadVector(ReaderValue, &Result.Color, 4);

  Result.Intensity = ReaderF32(ReaderValue);
  Result.Range = ReaderF32(ReaderValue);
  Result.IsSpot = IsSpot;

  if (IsSpot) {
    Result.Fov = ReaderF32(ReaderValue);
    Result.Face = (FaceId)ReaderU32(ReaderValue);
  }

  return Result;
}

static void WriteLight(Writer *WriterValue, const Light *LightValue,
                       bool IsSpot) {
  WriteVector(WriterValue, &LightValue->Color, 4);

  WriterF32(WriterValue, LightValue->Intensity);
  WriterF32(WriterValue, LightValue->Range);

  if (IsSpot) {
    WriterF32(WriterValue, LightValue->Fov);
    WriterU32(WriterValue, (uint32_t)LightValue->Face);
  }
}

static PartBody ReadPart(Reader *ReaderValue) {
  PartBody Body = {0};

  Body.ParentId = ReaderOptionalId(ReaderValue);

  Body.HasDisplayName = ReaderBool(ReaderValue);

  if (Body.HasDisplayName) {
    String Value = ReaderString(ReaderValue);
    Body.DisplayName = Value.Data;
  }

  ReadVector(ReaderValue, &Body.Position, 3);
  ReadVector(ReaderValue, &Body.Rotation, 4);
  ReadVector(ReaderValue, &Body.Size, 3);
  ReadVector(ReaderValue, &Body.Color, 4);

  Body.Material = (MaterialId)ReaderU32(ReaderValue);

  Body.Prefix = ReadRaw(ReaderValue, 1);

  Body.CastShadow = ReaderBool(ReaderValue);
  Body.Anchored = ReaderBool(ReaderValue);
  Body.CanCollide = ReaderBool(ReaderValue);
  Body.SpawnLocation = ReaderBool(ReaderValue);
  Body.Locked = ReaderBool(ReaderValue);
  Body.CustomAppearance = ReaderBool(ReaderValue);
  Body.Truss = ReaderBool(ReaderValue);

  uint64_t TextureCount = ReaderU64(ReaderValue);

  if (TextureCount > 1000000)
    Fail("[Vcgs] unreasonable texture count");

  Body.TextureCount = (size_t)TextureCount;

  if (Body.TextureCount != 0)
    Body.Textures = Allocate(sizeof(Texture) * Body.TextureCount);

  for (size_t Index = 0; Index < Body.TextureCount; Index++) {
    Body.Textures[Index].Face = (FaceId)ReaderU32(ReaderValue);

    Body.Textures[Index].Kind = ReaderU32(ReaderValue);
  }

  if (ReaderBool(ReaderValue)) {
    Body.PointLight = Allocate(sizeof(Light));
    *Body.PointLight = ReadLight(ReaderValue, false);
  }

  if (ReaderBool(ReaderValue)) {
    Body.SpotLight = Allocate(sizeof(Light));
    *Body.SpotLight = ReadLight(ReaderValue, true);
  }

  Body.Footer = ReadRaw(ReaderValue, 12);
  Body.Collapsed = Body.Footer.Data[Body.Footer.Length - 1] != 0;

  return Body;
}

static void WritePart(Writer *WriterValue, const PartBody *Body) {
  WriterOptionalId(WriterValue, Body->ParentId);

  WriterBool(WriterValue, Body->HasDisplayName);

  if (Body->HasDisplayName)
    WriterString(WriterValue, Body->DisplayName);

  WriteVector(WriterValue, &Body->Position, 3);
  WriteVector(WriterValue, &Body->Rotation, 4);
  WriteVector(WriterValue, &Body->Size, 3);
  WriteVector(WriterValue, &Body->Color, 4);

  WriterU32(WriterValue, (uint32_t)Body->Material);

  WriterWrite(WriterValue, Body->Prefix.Data, Body->Prefix.Length);

  WriterBool(WriterValue, Body->CastShadow);
  WriterBool(WriterValue, Body->Anchored);
  WriterBool(WriterValue, Body->CanCollide);
  WriterBool(WriterValue, Body->SpawnLocation);
  WriterBool(WriterValue, Body->Locked);
  WriterBool(WriterValue, Body->CustomAppearance);
  WriterBool(WriterValue, Body->Truss);

  WriterU64(WriterValue, Body->TextureCount);

  for (size_t Index = 0; Index < Body->TextureCount; Index++) {
    WriterU32(WriterValue, (uint32_t)Body->Textures[Index].Face);

    WriterU32(WriterValue, Body->Textures[Index].Kind);
  }

  WriterBool(WriterValue, Body->PointLight != NULL);

  if (Body->PointLight)
    WriteLight(WriterValue, Body->PointLight, false);

  WriterBool(WriterValue, Body->SpotLight != NULL);

  if (Body->SpotLight)
    WriteLight(WriterValue, Body->SpotLight, true);

  if (Body->Footer.Length != 12)
    Fail("[Vcgs] part footer must be 12 bytes");

  Bytes Footer = CopyBytes(Body->Footer.Data, Body->Footer.Length);

  Footer.Data[Footer.Length - 1] = Body->Collapsed ? 1 : 0;

  WriterWrite(WriterValue, Footer.Data, Footer.Length);

  FreeBytes(&Footer);
}

static ScriptBody ReadScript(Reader *ReaderValue, size_t RecordEnd) {
  ScriptBody Body = {0};

  Body.ParentId = ReaderOptionalId(ReaderValue);
  Body.UnknownPrefix = ReadRaw(ReaderValue, 3);

  Body.HasSource = ReaderBool(ReaderValue);

  if (Body.HasSource) {
    String Source = ReaderString(ReaderValue);
    Body.Source = Source.Data;
  }

  if (RecordEnd == SIZE_MAX || RecordEnd < ReaderValue->Position)
    Fail("[Vcgs] cannot find end of script record");

  size_t TailLength = RecordEnd - ReaderValue->Position;

  Body.Tail = ReadRaw(ReaderValue, TailLength);

  Body.Collapsed = TailLength != 0 && Body.Tail.Data[TailLength - 1] != 0;

  return Body;
}

static VariableBody ReadVariableBody(Reader *ReaderValue, size_t RecordEnd,
                                     ClassId Class) {
  VariableBody Body = {0};

  Body.ParentId = ReaderOptionalId(ReaderValue);

  size_t End = RecordEnd;

  if (End == SIZE_MAX)
    Fail("[Vcgs] cannot find end of record");

  if (End < ReaderValue->Position)
    Fail("[Vcgs] invalid record end");

  size_t Length = End - ReaderValue->Position;

  Bytes Raw = ReadRaw(ReaderValue, Length);

  if (Length != 0) {
    Body.HasCollapsed = true;
    Body.Collapsed = Raw.Data[Length - 1] != 0;
  }

  if (HasNoProperties(Class))
    Body.Footer = Raw;
  else
    Body.RawBody = Raw;

  return Body;
}

static Record ReadRecord(Reader *ReaderValue, bool HasRemainingRecords) {
  Record Result = {0};

  Result.ClassId = (ClassId)ReaderU32(ReaderValue);

  String Name = ReaderString(ReaderValue);
  Result.Name = Name.Data;

  size_t RecordEnd = SIZE_MAX;

  if (HasRemainingRecords) {
    RecordEnd = FindNextHeader(ReaderValue->Data, ReaderValue->Length,
                               ReaderValue->Position);

    if (RecordEnd == SIZE_MAX)
      Fail("[Vcgs] cannot find next record");
  } else {
    if (ReaderValue->Length < 57)
      Fail("[Vcgs] invalid payload");

    RecordEnd = ReaderValue->Length - 57;
  }

  if (Result.ClassId == ClassPart) {
    Result.BodyType = BodyPart;
    Result.Body.Part = ReadPart(ReaderValue);
  } else if (IsScriptClass(Result.ClassId)) {
    Result.BodyType = BodyScript;
    Result.Body.Script = ReadScript(ReaderValue, RecordEnd);
  } else {
    Result.BodyType = BodyVariable;
    Result.Body.Variable =
        ReadVariableBody(ReaderValue, RecordEnd, Result.ClassId);
  }

  return Result;
}

static void WriteRecord(Writer *WriterValue, const Record *RecordValue) {
  WriterU32(WriterValue, (uint32_t)RecordValue->ClassId);

  WriterString(WriterValue, RecordValue->Name);

  if (RecordValue->ClassId == ClassPart) {
    WritePart(WriterValue, &RecordValue->Body.Part);

    return;
  }

  if (IsScriptClass(RecordValue->ClassId)) {
    const ScriptBody *Body = &RecordValue->Body.Script;

    WriterOptionalId(WriterValue, Body->ParentId);

    WriterWrite(WriterValue, Body->UnknownPrefix.Data,
                Body->UnknownPrefix.Length);

    WriterBool(WriterValue, Body->HasSource);

    if (Body->HasSource)
      WriterString(WriterValue, Body->Source);

    WriterWrite(WriterValue, Body->Tail.Data, Body->Tail.Length);

    return;
  }

  const VariableBody *Body = &RecordValue->Body.Variable;

  WriterOptionalId(WriterValue, Body->ParentId);

  if (Body->Footer.Length != 0) {
    WriterWrite(WriterValue, Body->Footer.Data, Body->Footer.Length);
  } else {
    WriterWrite(WriterValue, Body->RawBody.Data, Body->RawBody.Length);
  }
}

static Document DecodePayload(const uint8_t *Payload, size_t PayloadLength) {
  Reader ReaderValue = {
      .Data = Payload, .Length = PayloadLength, .Position = 0};

  Document DocumentValue = {0};

  DocumentValue.Version = ReaderU8(&ReaderValue);

  DocumentValue.ProjectId = ReaderString(&ReaderValue);

  uint64_t RecordCount = ReaderU64(&ReaderValue);

  if (RecordCount > SIZE_MAX / sizeof(Record))
    Fail("[Vcgs] too many records");

  DocumentValue.RecordCount = (size_t)RecordCount;

  if (DocumentValue.RecordCount != 0) {
    DocumentValue.Records = calloc(DocumentValue.RecordCount, sizeof(Record));
  }

  for (size_t Index = 0; Index < DocumentValue.RecordCount; Index++) {
    DocumentValue.Records[Index] =
        ReadRecord(&ReaderValue, Index + 1 < DocumentValue.RecordCount);
  }

  ReadVector(&ReaderValue, &DocumentValue.Lighting.AmbientColor, 4);

  DocumentValue.Lighting.Brightness = ReaderF32(&ReaderValue);

  ReadVector(&ReaderValue, &DocumentValue.Lighting.SunColor, 4);

  DocumentValue.Lighting.SunIlluminance = ReaderF32(&ReaderValue);

  DocumentValue.Lighting.SunShadowMapsEnabled = ReaderBool(&ReaderValue);

  ReadVector(&ReaderValue, &DocumentValue.Lighting.SunDirection, 4);

  if (ReaderValue.Position < ReaderValue.Length) {
    DocumentValue.TrailingBytes =
        ReadRaw(&ReaderValue, ReaderValue.Length - ReaderValue.Position);
  }

  return DocumentValue;
}

static ByteBuffer EncodePayload(const Document *DocumentValue) {
  Writer WriterValue = {0};

  WriterU8(&WriterValue, DocumentValue->Version);

  WriterString(&WriterValue, DocumentValue->ProjectId.Data);

  WriterU64(&WriterValue, DocumentValue->RecordCount);

  for (size_t Index = 0; Index < DocumentValue->RecordCount; Index++) {
    WriteRecord(&WriterValue, &DocumentValue->Records[Index]);
  }

  WriteVector(&WriterValue, &DocumentValue->Lighting.AmbientColor, 4);

  WriterF32(&WriterValue, DocumentValue->Lighting.Brightness);

  WriteVector(&WriterValue, &DocumentValue->Lighting.SunColor, 4);

  WriterF32(&WriterValue, DocumentValue->Lighting.SunIlluminance);

  WriterBool(&WriterValue, DocumentValue->Lighting.SunShadowMapsEnabled);

  if (DocumentValue->Lighting.SunDirection.Count == 4) {
    WriteVector(&WriterValue, &DocumentValue->Lighting.SunDirection, 4);
  } else {
    WriterWrite(&WriterValue, DocumentValue->UnknownQuat.Data,
                DocumentValue->UnknownQuat.Length);
  }

  WriterWrite(&WriterValue, DocumentValue->TrailingBytes.Data,
              DocumentValue->TrailingBytes.Length);

  return WriterValue.Data;
}

static bool StartsWithVrtx(const uint8_t *Data, size_t Length) {
  return Length >= 5 && Data[0] == 'V' && Data[1] == 'R' && Data[2] == 'T' &&
         Data[3] == 'X';
}

DecodedFile DecodeVrtxFile(const uint8_t *Data, size_t Length) {
  DecodedFile Result = {0};

  if (!StartsWithVrtx(Data, Length)) {
    Result.Document = DecodePayload(Data, Length);

    Result.Compression.Kind = CompressionRaw;

    return Result;
  }

  if (Length < 6)
    Fail("[Vcgs] truncated VRTX wrapper");

  size_t CompressedLength = Length - 5;

  ZSTD_DStream *Stream = ZSTD_createDStream();

  if (Stream == NULL)
    Fail("[Vcgs] unable to create zstd stream");

  size_t InitResult = ZSTD_initDStream(Stream);

  if (ZSTD_isError(InitResult))
    Fail(ZSTD_getErrorName(InitResult));

  ZSTD_inBuffer Input = {.src = Data + 5, .size = CompressedLength, .pos = 0};

  ByteBuffer Payload = {0};

  while (Input.pos < Input.size) {
    uint8_t Buffer[65536];

    ZSTD_outBuffer Output = {.dst = Buffer, .size = sizeof(Buffer), .pos = 0};

    size_t ResultSize = ZSTD_decompressStream(Stream, &Output, &Input);

    if (ZSTD_isError(ResultSize)) {
      ZSTD_freeDStream(Stream);
      BufferFree(&Payload);
      Fail(ZSTD_getErrorName(ResultSize));
    }

    BufferAppend(&Payload, Buffer, Output.pos);

    if (ResultSize == 0 && Input.pos == Input.size)
      break;
  }

  ZSTD_freeDStream(Stream);

  Result.Document = DecodePayload(Payload.Data, Payload.Length);

  BufferFree(&Payload);

  return Result;
}

EncodedFile EncodeVrtxFile(const Document *DocumentValue) {
  EncodedFile Result = {0};

  ByteBuffer Payload = EncodePayload(DocumentValue);

  size_t CompressionBound = ZSTD_compressBound(Payload.Length);

  uint8_t *Compressed = Allocate(CompressionBound);

  size_t CompressedLength = ZSTD_compress(Compressed, CompressionBound,
                                          Payload.Data, Payload.Length, 19);

  if (ZSTD_isError(CompressedLength)) {
    free(Compressed);

    Result.Data = Payload.Data;
    Result.Length = Payload.Length;
    Result.Compressed = false;

    return Result;
  }

  Result.Length = 5 + CompressedLength;

  Result.Data = Allocate(Result.Length);

  Result.Data[0] = 'V';
  Result.Data[1] = 'R';
  Result.Data[2] = 'T';
  Result.Data[3] = 'X';
  Result.Data[4] = 4;

  memcpy(Result.Data + 5, Compressed, CompressedLength);

  Result.Compressed = true;

  free(Compressed);
  BufferFree(&Payload);

  return Result;
}

static void FreeLight(Light *Value) {
  if (!Value)
    return;

  FreeFloatArray(&Value->Color);
  free(Value);
}

static void FreePartBody(PartBody *Body) {
  FreeFloatArray(&Body->Position);
  FreeFloatArray(&Body->Rotation);
  FreeFloatArray(&Body->Size);
  FreeFloatArray(&Body->Color);

  free(Body->DisplayName);

  FreeBytes(&Body->Prefix);
  FreeBytes(&Body->Footer);

  free(Body->Textures);

  FreeLight(Body->PointLight);
  FreeLight(Body->SpotLight);

  Body->Textures = NULL;
  Body->TextureCount = 0;
  Body->PointLight = NULL;
  Body->SpotLight = NULL;
}

static void FreeScriptBody(ScriptBody *Body) {
  free(Body->Source);

  FreeBytes(&Body->UnknownPrefix);
  FreeBytes(&Body->Tail);

  Body->Source = NULL;
}

static void FreeVariableBody(VariableBody *Body) {
  FreeBytes(&Body->Footer);
  FreeBytes(&Body->RawBody);
}

static void FreeRecord(Record *RecordValue) {
  free(RecordValue->Name);

  switch (RecordValue->BodyType) {
  case BodyPart:
    FreePartBody(&RecordValue->Body.Part);
    break;

  case BodyScript:
    FreeScriptBody(&RecordValue->Body.Script);
    break;

  case BodyVariable:
    FreeVariableBody(&RecordValue->Body.Variable);
    break;
  }
}

static void FreeDocument(Document *DocumentValue) {
  free(DocumentValue->ProjectId.Data);

  for (size_t Index = 0; Index < DocumentValue->RecordCount; Index++) {
    FreeRecord(&DocumentValue->Records[Index]);
  }

  free(DocumentValue->Records);

  FreeFloatArray(&DocumentValue->Lighting.AmbientColor);

  FreeFloatArray(&DocumentValue->Lighting.SunColor);

  FreeFloatArray(&DocumentValue->Lighting.SunDirection);

  FreeBytes(&DocumentValue->UnknownQuat);
  FreeBytes(&DocumentValue->TrailingBytes);

  memset(DocumentValue, 0, sizeof(*DocumentValue));
}

void FreeDecodedFile(DecodedFile *Value) { FreeDocument(&Value->Document); }

void FreeEncodedFile(EncodedFile *Value) {
  free(Value->Data);

  Value->Data = NULL;
  Value->Length = 0;
  Value->Compressed = false;
}
