# Day 02a Log

## Repository Exploration
- リポジトリ直下の `run.sh` は `$HOME/osbook/devenv/run_image.sh` を実行して QEMU で MikanOS を起動。
- `build.sh` はカーネル／アプリをビルドし、`make_mikanos_image.sh` で FAT ディスクイメージを生成。`run` 引数で QEMU を起動。
- `MikanLoaderPkg/Main.c` が UEFI アプリ本体。最小構成では "Hello, Mikan World!" と表示して無限ループ。

## UEFI / ブートローダ理解
- `UefiMain` が UEFI アプリのエントリポイント。UEFI は `EFI_HANDLE` と `EFI_SYSTEM_TABLE*` を渡す。
- `Print` は UefiLib の関数で UTF-16 出力。UEFI が提供するコンソールへ表示。
- `<Library/UefiLib.h>` などのヘッダは EDK II のライブラリであり、標準 C ライブラリとは別物。
- `EFI_STATUS` は UEFI の戻り値型、`EFIAPI` は UEFI 呼び出し規約を示すマクロ。
- `EFI_STATUS EFIAPI UefiMain(...)` は通常の `main` に相当し、UEFI 環境でのみ呼ばれる。

## Git / CLI Tips
- タグを checkout した状態で作業していたため detached HEAD。`git checkout -b dev` → `git push -u origin dev` で作業ブランチを作成。
- 現在のタグ確認は `git tag --points-at HEAD`、距離付き表示は `git describe --tags`。
- GitHub CLI の参照先切替: `gh browse --repo owner/repo` または `gh repo set-default owner/repo`。

## QEMU / 実行環境メモ
- 起動が遅いのは OVMF の Boot Manager Timeout のため。設定で短縮可能。
- `-nographic -serial mon:stdio` で GUI なし起動。`Print` の内容がターミナルに直接出力。
- macOS では `-accel hvf` で高速化可。GUI 表示なしのほうが軽快。

## OS/UEFI 学習メモ
- ブートローダは: メモリマップ取得 → FAT から `kernel.elf` 読込 → ページ確保 → `CopyLoadSegments` → `ExitBootServices` → カーネル呼び出し。
- UEFI のファイルシステムは OVMF が提供する FAT ボリューム。自作 FS はまだ不要。
- `gBS` は `EFI_BOOT_SERVICES*`。`gBS->GetMemoryMap` などでブートサービスを呼び出す。
- TianoCore プロジェクトは EDK II／OVMF を提供する UEFI リファレンス実装。多くの製品がベースとして利用。

## GetMemoryMap / SaveMemoryMap の理解
- `GetMemoryMap(struct MemoryMap* map)` はバッファサイズを `map_size` に設定し、`gBS->GetMemoryMap` でメモリマップを取得するラッパ。
- `MemoryMap` 構造体は `buffer`, `buffer_size`, `map_size`, `map_key`, `descriptor_size`, `descriptor_version` を保持。
- `GetMemoryTypeUnicode` は `EFI_MEMORY_TYPE` の列挙値を UTF-16 の文字列に変換。
- `OpenRootDir` は `EFI_LOADED_IMAGE_PROTOCOL` と `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL` を開いてルートディレクトリ (`EFI_FILE_PROTOCOL*`) を取得。
- `SaveMemoryMap` はヘッダを書き出し、`EFI_MEMORY_DESCRIPTOR` を走査して CSV 行を生成 (`AsciiSPrint`) → `file->Write` で保存。

## メモリマップ走査の詳細
- `EFI_PHYSICAL_ADDRESS iter = (EFI_PHYSICAL_ADDRESS)map->buffer;` でバッファ先頭アドレスを保持。
- ループ条件は `iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size`。`map->descriptor_size` ずつ進めて各エントリを処理。
- `EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;` を介して `PhysicalStart`, `NumberOfPages`, `Attribute` 等を参照。
- 出力結果は `disk.img` 内の `\memmap`。`mcopy -i disk.img ::memmap ./memmap.txt` 等で抽出可。

## OS 全般に関するメモ
- ターミナル/シェル/ユーザランドとカーネルの役割の違い (`cat`, `less`, `kill` などはユーザランドアプリ)。
- カーネルはプロセス管理・メモリ管理・デバイス制御・システムコールなど基盤機能を担当。生命維持装置の比喩。
- ページングの重要性と、非採用時の問題点（保護ができない、断片化、スワップ不可等）。
- xv6 や Minix の規模感、学習用コンパイラ (`chibicc` など) や自作 VM の構成要素。
- バイナリ解析手法（`objdump`, `nm`, `gdb`）やコンテナ内でのカーネル観察（`/proc`, `perf`, eBPF）。

## 作業メモ
- `logs/day_02b_log.md` を作成。`dev` ブランチで管理中。
- 次の目標: メモリマップ取得コードを写経し、CSV を確認する。
