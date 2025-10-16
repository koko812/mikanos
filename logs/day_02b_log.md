# Day 02b Log

## 今日のテーマ
- `MikanLoaderPkg/Main.c` のフル実装版を読み解き、メモリマップ取得〜保存の流れを理解する。
- UEFI 固有の型／API（`EFI_STATUS`, `EFI_BOOT_SERVICES`, `EFI_FILE_PROTOCOL` など）の使われ方を整理。

## GetMemoryMap ラッパの理解
- `struct MemoryMap`（`memory_map.hpp` 定義）はメモリマップ取得に必要なバッファ情報・キー・ディスクリプタサイズを保持。
- `GetMemoryMap(struct MemoryMap* map)` は
  - バッファ未確保 (`map->buffer == NULL`) の場合は `EFI_BUFFER_TOO_SMALL` を返す早期チェック。
  - 呼び出し前に `map_size` を `buffer_size` で初期化し、`gBS->GetMemoryMap` を呼んで実データ・`map_key`・`descriptor_size` を埋め戻す。
- `gBS` は `EFI_BOOT_SERVICES*` のグローバル。`gBS->GetMemoryMap` のように、構造体に格納された関数ポインタとして呼び出す。

## GetMemoryTypeUnicode
- `EFI_MEMORY_TYPE` の列挙値を人間可読な `CHAR16` 文字列へ変換するヘルパー。
- CSV 出力時に数値だけでなく名前も併記するために使用。

## SaveMemoryMap の詳細
- `EFI_FILE_PROTOCOL* file` を通じて FAT 上のファイルへ書き出す。
- ASCII ヘッダ行 → 各ディスクリプタ行の順で `file->Write` により保存。
- ループ処理:
  - `EFI_PHYSICAL_ADDRESS iter = (EFI_PHYSICAL_ADDRESS)map->buffer;` で先頭アドレスを 64bit 値として保持。
  - 条件 `iter < (EFI_PHYSICAL_ADDRESS)map->buffer + map->map_size` で配列末尾まで反復。
  - 各ステップで `EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)iter;` にキャストし、`PhysicalStart`・`NumberOfPages`・`Attribute` などを `AsciiSPrint` で整形。
  - `map->descriptor_size` ずつ `iter` を進め、ディスクリプタ配列を走査。
- 出力ファイル `\memmap` は FAT イメージ内に残る。`mcopy -i disk.img ::memmap ./memmap.txt` でホストへコピー可能。

## OpenRootDir とファイル操作
- `OpenRootDir`:
  - `EFI_LOADED_IMAGE_PROTOCOL` を開いて自身が載っているデバイスハンドルを取得。
  - そのデバイスに対して `EFI_SIMPLE_FILE_SYSTEM_PROTOCOL` を開き、`OpenVolume` でルートディレクトリの `EFI_FILE_PROTOCOL*` を受け取る。
- `EFI_FILE_PROTOCOL` は `Write`, `Read`, `Close` などのメソッドを持つ構造体。C では関数ポインタ呼び出しの形で `file->Write(file, &len, buf);` と使う。

## ポインタと型の補足
- `CHAR8`, `UINTN`, `EFI_PHYSICAL_ADDRESS` などは EDK II が定義する UEFI 専用型。
- `EFI_STATUS` は 64bit 整数型で成功/エラーコードを表し、`EFI_SUCCESS` 等のマクロとセットで利用。
- `EFIAPI` は UEFI 規定の呼び出し規約を示すマクロ。
- `struct MemoryMap memmap` のように typedef がなければ `struct` キーワードを付けて宣言する。

## その他の気づき
- 取得・保存処理はすべて UEFI ブートサービス上で完結。`ExitBootServices` 後は利用不可になるため、カーネルに渡す前の準備段階として実行。
- FAT ボリュームは OVMF が提供。ブートローダは既存の UEFI プロトコルを介してアクセスするだけで、自前実装は不要。
- ログ作成のため `logs/day_02a_log.md`（前日の内容）と今回の `logs/day_02b_log.md` を整理。

## 次のアクション
- コードを写経して実際に `\memmap` を生成・抽出し、中身を確認する。
- `OpenGOP` や `CopyLoadSegments` など未読の関数についても同様に分解して理解を進める。
