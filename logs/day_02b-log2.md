## Overview (10/15 22:41)
- 既存の `Main.c` を大幅に書き換え、UEFI ブートローダとしてメモリマップ取得・保存の処理を実装。
- QEMU + OVMF 上で動作させ、`\\memmap` を生成＆ホストへ抽出 (`mcopy`) して内容を確認。
- UEFI プロトコル、ポインタ操作、`EFI_MEMORY_DESCRIPTOR`、ページ単位のメモリ管理などを深掘り。
- カーネル／ユーザモード、システムコール、ページング、メモリ保護など OS 全体の概念も整理。

## Main.c の主な実装メモ
- 構造体 `Memory_Map` を定義し、`buffer_size`, `buffer`, `map_size`, `map_key`, `descriptor_size`, `descriptor_version (UINT32)` を保持。
- `GetMemoryMap`:
  - バッファ未確保チェック (`EFI_BUFFER_TOO_SMALL`)。
  - `map_size = buffer_size` → `gBS->GetMemoryMap` 呼び出し。
  - `gBS` は `EFI_BOOT_SERVICES*` のグローバル。関数ポインタ経由で UEFI API を実行。
- `MemoryKeyToUnicode`（`const CHAR16*`）で `EFI_MEMORY_TYPE` を判別し、名称を返す。
- `Save_Memorymap`:
  - `AsciiSPrint` で CSV 行をバッファに整形。
  - `file->Write(file, &len, buf)` で FAT 上の `\\memmap` に追記。
  - `EFI_MEMORY_DESCRIPTOR` 配列を `EFI_PHYSICAL_ADDRESS iter` で走査し、ページ数 (`NumberOfPages` × 0x1000) を含めて出力。
- `OpenRootDir`:
  - `EFI_LOADED_IMAGE_PROTOCOL` と `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL` を `OpenProtocol` で取得。
  - ルートディレクトリの `EFI_FILE_PROTOCOL*` を返す（二重ポインタで書き込ませる）。
- `UefiMain`:
  - `CHAR8 memmap_buf[4096*4]` を用意。
  - `GetMemoryMap` → `OpenRootDir` → `root_dir->Open(L"\\memmap")` → `Save_Memorymap`。
  - デバッグ用に `map->buffer` と `map->map_size` を `Print`。
  - 終了後 `while (1);` で停止。

## Loader.inf の更新
- `[Protocols]` に `gEfiLoadedImageProtocolGuid`, `gEfiSimpleFileSystemProtocolGuid`, `gEfiBlockIoProtocolGuid` を追加。
- 必要なヘッダ（`<Protocol/LoadedImage.h>` 等）をインクルードし、リンクエラーを解消。

## ビルドと実行
- Docker 内で `./build.sh` → `./run.sh` により QEMU を起動。
- ホスト macOS では `qemu-system-x86_64 -drive if=pflash,... -drive file=disk.img -serial stdio -monitor stdio -nographic` 等で直接起動（Apple Silicon は `-accel hvf` 非対応）。
- `mcopy -i disk.img ::memmap ./memmap.txt` でブート後に生成された CSV を抽出。

## メモリマップ観察
- `NumberOfPages` は 4KiB 単位。`PhysicalStart` の差分は `ページ数 × 0x1000`。
- 同じ `Type` が複数回出るのは正常。領域の役割が複数存在するため。
- UEFI の返す順序は必ずしもソートされていない。完全な一覧は `memmap.txt` で確認。
- `EFI_MEMORY_TYPE` の分類を把握し、`EfiConventionalMemory` など OS が利用可能な領域を理解。

## ポインタと型の整理
- `*` はポインタ宣言／デリファレンス、`&` はアドレス取得。更新してほしい値は `&` で渡す。
- `Memory_Map` は実体なので `&memmap` を渡す。`EFI_FILE_PROTOCOL*` はポインタそのものを渡す。
- `OpenRootDir` は `EFI_FILE_PROTOCOL**` を受け、呼び出し元のポインタに値を書き込む。
- `descriptor_version` は `UINT32` で受ける必要がある（`UINTN` では型不一致）。

## UEFI / OS 概念メモ
- プロトコルは GUID に紐づくインターフェース構造体（関数ポインタ群）。`OpenProtocol` で実体を取得。
- `EFI_FILE_PROTOCOL` などはコピーせずポインタで扱うのが前提。
- メモリマップはブート時に Live で取得する情報で、事前には分からない。
- 保護すべき領域 (`EfiRuntimeServicesData`, `EfiACPIReclaimMemory` など) は OS が再利用しないよう管理。
- ページングは OS の土台。ブートローダは UEFI に頼るが、カーネルで自前のページ管理へ移行予定。
- ラージページ（2MiB/1GiB）も存在するが、UEFI の `NumberOfPages` は 4KiB ページ換算。

## カーネルモード / ユーザモードの整理
- `printf` → `write` システムコール → カーネルモードでデバイス I/O → ユーザモードへ戻る流れ。
- 動画再生など他のアプリも同様に I/O の瞬間だけカーネルモードを経由。
- カーネルモードは危険だが必要。ページングと保護でユーザランドを隔離。
- 初期化フェーズでは保護が効かないため、バッファオーバーフロー等に要注意。

## 感想
- OS 開発はハードとソフトの境界が曖昧で、ハードウェアを直接いじる感覚が強い。
- UEFI に頼りつつも、メモリマップ取得など低レイヤの世界へ一歩踏み込んだ実感。
- 今後は `kernel.elf` ロード、`ExitBootServices`、ページング設定など本格的な OS 開発へ進む。

## 次にやること
1. `Main.c` をリファクタリング（スペル／ログ調整、不要なコードの削除）。
2. `kernel.elf` の読み込み・ページ確保・`CopyLoadSegments` の実装。
3. `ExitBootServices` 後にカーネルのエントリポイントへ制御を渡す処理を追加。
4. 取得したメモリマップをカーネルへ渡し、物理メモリ管理へ活用。