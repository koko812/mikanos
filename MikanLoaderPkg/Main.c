#include  <Uefi.h>
#include  <Library/UefiLib.h>

EFI_STATUS EFIAPI UefiMain(
    EFI_HANDLE image_handle,
    EFI_SYSTEM_TABLE *system_table) {
  Print(L"Hello, Iwakawa World!\n");
  while (1);
  return EFI_SUCCESS;
}


EFI_STATUS GetMemoryMap(struct Memory* map){
  if(map->buffer == NULL){
    return EFI_BUFFER_TOO_SMALL;
  }
  map-> map_size = map -> buffer_size;
  return gBS->GetMemoryMap(
    &map->map_size,
    (EFI_MEMORY_DESCRIPTOR*)map->buffer,
    &map->map_key,
    &map->descriptor_size,
    &map->descriptor_version
  );
}

