#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "vcgs.h"

int main(void) {
  FILE *FilePtr = fopen("examples/baseplate.vrtx", "rb");
  if (!FilePtr) {

    return -1;
  }

  fseek(FilePtr, 0, SEEK_END);
  long FileSize = ftell(FilePtr);
  fseek(FilePtr, 0, SEEK_SET);

  size_t DataLength = (size_t)FileSize;
  uint8_t *Data = Allocate(DataLength);

  if (fread(Data, 1, DataLength, FilePtr) != DataLength) {
    free(Data);
    fclose(FilePtr);
    return -1;
  }

  fclose(FilePtr);

  DecodedFile File = DecodeVrtxFile(Data, DataLength);
  for (size_t Index = 0; Index < File.Document.RecordCount; Index++) {
    Record *RecordValue = &File.Document.Records[Index];

    if (RecordValue->ClassId != ClassPart)
      continue;

    PartBody *Part = &RecordValue->Body.Part;

    printf("Part: %s\n", RecordValue->Name);
    printf("Position: %f, %f, %f\n", Part->Position.Values[0],
           Part->Position.Values[1], Part->Position.Values[2]);
  }

  if (WriteGameJson(&File.Document, "web/game.json") != 0) {
    Fail("[Vcgs] Failed to write /web/game.json\n");
    return -1;
  }

  ServerStart();

  FreeDecodedFile(&File);
  free(Data);

  return 0;
}
