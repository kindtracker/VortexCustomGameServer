#pragma once
#include <stdbool.h>

typedef struct {
  uint8_t *Data;
  size_t Length;
  size_t Capacity;
} ByteBuffer;

typedef struct {
  const uint8_t *Data;
  size_t Length;
  size_t Position;
} Reader;

typedef struct {
  ByteBuffer Data;
} Writer;

typedef struct {
  float *Values;
  size_t Count;
} FloatArray;

typedef struct {
  uint64_t Value;
  bool HasValue;
} OptionalId;

typedef struct {
  char *Data;
  size_t Length;
} String;

typedef struct {
  uint8_t *Data;
  size_t Length;
} Bytes;

typedef enum {
  ClassWorkspace = 0,
  ClassLighting = 1,
  ClassPart = 2,
  ClassGroup = 3,
  ClassFolder = 4,
  ClassIntValue = 5,
  ClassStringValue = 6,
  ClassLocalScript = 7,
  ClassScript = 8,
  ClassModuleScript = 9,
  ClassReplicatedStorage = 10,
  ClassStarterPlayerScripts = 11,
  ClassServerScriptService = 12,
  ClassRemoteEvent = 13,
  ClassBindableEvent = 14,
  ClassRemoteFunction = 15,
  ClassBindableFunction = 16
} ClassId;

typedef enum {
  MaterialSmooth = 0,
  MaterialPlastic = 2,
  MaterialWood = 3,
  MaterialMetal = 4,
  MaterialGrass = 5,
  MaterialIce = 6,
  MaterialPaint = 7
} MaterialId;

typedef enum {
  FaceRight = 0,
  FaceTop = 1,
  FaceBack = 2,
  FaceLeft = 3,
  FaceBottom = 4,
  FaceFront = 5
} FaceId;

typedef struct {
  FloatArray Color;
  float Intensity;
  float Range;
  float Fov;
  FaceId Face;
  bool IsSpot;
} Light;

typedef struct {
  FaceId Face;
  uint32_t Kind;
} Texture;

typedef struct {
  OptionalId ParentId;
  char *DisplayName;
  bool HasDisplayName;

  FloatArray Position;
  FloatArray Rotation;
  FloatArray Size;
  FloatArray Color;

  MaterialId Material;

  Bytes Prefix;

  bool CastShadow;
  bool Anchored;
  bool CanCollide;
  bool SpawnLocation;
  bool Locked;
  bool CustomAppearance;
  bool Truss;

  Texture *Textures;
  size_t TextureCount;

  Light *PointLight;
  Light *SpotLight;

  bool Collapsed;
  Bytes Footer;
} PartBody;

typedef struct {
  OptionalId ParentId;
  Bytes UnknownPrefix;

  char *Source;
  bool HasSource;

  bool Collapsed;
  Bytes Tail;
} ScriptBody;

typedef struct {
  OptionalId ParentId;

  bool HasCollapsed;
  bool Collapsed;

  Bytes Footer;
  Bytes RawBody;
} VariableBody;

typedef enum { BodyPart, BodyScript, BodyVariable } BodyType;

typedef struct {
  ClassId ClassId;
  char *Name;
  BodyType BodyType;

  union {
    PartBody Part;
    ScriptBody Script;
    VariableBody Variable;
  } Body;
} Record;

typedef struct {
  FloatArray AmbientColor;
  float Brightness;
  FloatArray SunColor;
  float SunIlluminance;
  bool SunShadowMapsEnabled;
  FloatArray SunDirection;
} Lighting;

typedef struct {
  uint8_t Version;
  String ProjectId;

  Record *Records;
  size_t RecordCount;

  Lighting Lighting;

  Bytes UnknownQuat;
  Bytes TrailingBytes;
} Document;

typedef enum { CompressionRaw, CompressionNvtZstd } CompressionKind;

typedef struct {
  CompressionKind Kind;
  uint8_t WrapperVersion;
} Compression;

typedef struct {
  Document Document;
  Compression Compression;
} DecodedFile;

typedef struct {
  uint8_t *Data;
  size_t Length;
  bool Compressed;
} EncodedFile;

DecodedFile DecodeVrtxFile(const uint8_t *Data, size_t Length);
EncodedFile EncodeVrtxFile(const Document *DocumentValue);
void FreeDecodedFile(DecodedFile *Value);
void FreeEncodedFile(EncodedFile *Value);

int WriteGameJson(const Document *DocumentValue, const char *Path);

int ServerStart(void);

void Fail(const char *Message);
void *Allocate(size_t Size);
void *Reallocate(void *Pointer, size_t Size);
char *DuplicateString(const char *Value);
Bytes CopyBytes(const uint8_t *Data, size_t Length);
FloatArray CreateFloatArray(size_t Count);
void FreeFloatArray(FloatArray *Array);
void FreeBytes(Bytes *Value);
void BufferReserve(ByteBuffer *Buffer, size_t Additional);
void BufferWrite(ByteBuffer *Buffer, const void *Data, size_t Length);
void BufferWriteU8(ByteBuffer *Buffer, uint8_t Value);
void BufferAppend(ByteBuffer *Buffer, const uint8_t *Data, size_t Length);
void BufferFree(ByteBuffer *Buffer);
uint32_t ReadU32Le(const uint8_t *Data);
int32_t ReadI32Le(const uint8_t *Data);
uint64_t ReadU64Le(const uint8_t *Data);
float ReadF32Le(const uint8_t *Data);
void WriteU32Le(uint8_t *Data, uint32_t Value);
void WriteI32Le(uint8_t *Data, int32_t Value);
void WriteU64Le(uint8_t *Data, uint64_t Value);
void WriteF32Le(uint8_t *Data, float Value);
