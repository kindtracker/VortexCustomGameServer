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
  EncodedFile Output = EncodeVrtxFile(&File.Document);

  FILE *OutputFile = fopen("output.vrtx", "wb");
  if (!OutputFile)
    return -1;

  fwrite(Output.Data, 1, Output.Length, OutputFile);
  fclose(OutputFile);

  FreeEncodedFile(&Output);
  FreeDecodedFile(&File);
  free(Data);

  return 0;
}
