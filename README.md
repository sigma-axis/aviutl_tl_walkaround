# TLショトカ移動 AviUtl プラグイン

タイムライン上のオブジェクト境界に移動する，いわゆる編集点移動のショートカットコマンドを追加するプラグインです．他にもスクロール操作などタイムラインをキーボード操作で見て回るのに便利なショートカットコマンドを追加します．

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_tl_walkaround/releases)

![編集点移動のデモ](https://github.com/sigma-axis/aviutl_tl_walkaround/assets/132639613/74b8bd73-1551-4068-b7bf-ca8ac2fb95b0)

## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl

  - 拡張編集 0.93rc1 等の他バーションでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

## 導入方法

以下のフォルダのいずれかに `tl_walkaround.auf` と `tl_walkaround.ini` をコピーしてください．

1. `aviutl.exe` のあるフォルダ
1. (1) のフォルダにある `plugins` フォルダ
1. (2) のフォルダにある任意のフォルダ

## 使い方

以下のコマンドが追加されます．

![編集→TLショトカ移動のメニュー](https://github.com/sigma-axis/aviutl_tl_walkaround/assets/132639613/25678c54-89c2-48ea-afc3-094fe3dba913)

AviUtlのメニューから「ファイル」→「環境設定」→「ショートカットキーの設定」とたどって，これらのコマンドを好みのキーに割り当ててください．

以下，各種コマンドの解説．

### 左の中間点(レイヤー) / 右の中間点(レイヤー) / 左のオブジェクト境界(レイヤー) / 右のオブジェクト境界(レイヤー)

現在選択オブジェクトと同じレイヤーから，現在フレームに最も近い左(右)にある中間点やオブジェクト境界を探して移動します．

- 「中間点」のものは中間点とオブジェクト境界のどちらにも止まります．「オブジェクト境界」のものは中間点は無視してスキップします．

- 選択オブジェクトがない場合，代わりに[左の中間点(シーン) / 右の中間点(シーン) / 左のオブジェクト境界(シーン) / 右のオブジェクト境界(シーン)](#左の中間点シーン--右の中間点シーン--左のオブジェクト境界シーン--右のオブジェクト境界シーン)を実行してシーン全体から境界を探します．

### 左の中間点(シーン) / 右の中間点(シーン) / 左のオブジェクト境界(シーン) / 右のオブジェクト境界(シーン)

現在のシーン全体から，現在フレームに最も近い左(右)にある中間点やオブジェクト境界を探して移動します．

- [左の中間点(レイヤー) / 右の中間点(レイヤー) / 左のオブジェクト境界(レイヤー) / 右のオブジェクト境界(レイヤー)](#左の中間点レイヤー--右の中間点レイヤー--左のオブジェクト境界レイヤー--右のオブジェクト境界レイヤー)とは異なり，現在選択オブジェクトのレイヤーによらずシーン全体から境界を探します．

### 現在位置を選択中間点区間に移動 / 現在位置を選択オブジェクトに移動

マウスクリックなどで現在フレーム上にない中間点区間やオブジェクトが選択中の場合，現在フレームを移動してその選択オブジェクトの上まで持ってきます．

### 現在位置をTL表示範囲内に移動

スクロール移動などで現在フレームがタイムラインの表示範囲外にある場合，現在フレームを移動して表示範囲内まで持ってきます．

- 表示範囲ギリギリから少し余裕を持った内側の表示位置に移動します．

### 左へ一定量移動 / 右へ一定量移動

左(右)へ画面上一定距離だけ現在フレームを移動します．

- 実際の移動フレーム数はタイムラインの拡大率に応じて変化します．

- 移動量は初期設定だと，タイムラインをホイールスクロールしたときの移動量と同じです．この移動量は[設定で調節](#設定ファイルについて)できます．

### 左へ1ページ分移動 / 右へ1ページ分移動

左(右)へタイムラインの表示横幅分だけ移動します．

- 実際は表示横幅より少し余裕を持った小さい量だけ移動します．

### TLスクロール(左) / TLスクロール(右)

タイムラインの水平スクロールをショートカットキーで行います．

- スクロール量は初期状態だとタイムラインをホイールスクロールしたときの移動量と同じです．この移動量は[設定で調節](#設定ファイルについて)できます．

### TLスクロール(左ページ) / TLスクロール(右ページ)

タイムラインの水平スクロールを表示横幅分だけ行います．

- 実際は表示横幅より少し余裕を持った小さい量だけ移動します．

### TLスクロール(現在位置まで)

スクロール移動などで現在フレームがタイムラインの表示範囲外にある場合，タイムラインをスクロールして現在フレームが表示される位置まで持っていきます．

- 表示範囲ギリギリから少し余裕を持った内側に表示されるよう移動します．

### TLスクロール(開始位置まで) / TLスクロール(終了位置まで)

タイムラインをシーンの開始(終了)位置までスクロールします．

### TLスクロール(上) / TLスクロール(下)

タイムラインの垂直スクロール(レイヤー移動)をショートカットキーで行います．

- スクロール量は初期状態だと1レイヤー分ですが，この移動量は[設定で調節](#設定ファイルについて)できます．

- これがなくても上下キーで移動可能ですが，タイムラインのウィンドウにフォーカスがないと機能しません．そこでショートカットキーを上下キーに登録すれば他のウィンドウにフォーカスがあっても機能するという算段です．また，他のキーに割り当てて複数レイヤー移動のコマンドにすることも可能です．

### 現在フレームのオブジェクトへ移動(上) / 現在フレームのオブジェクトへ移動(下)

拡張編集に標準であるコマンド「現在フレームのオブジェクトへ移動」(初期設定だと `TAB` キー)の上下の移動方向を指定したタイプです．

標準であるのものだと `Shift` キーを押してるかどうかで上下移動が切り替わる仕様ですが，これだと `Shift`, `Ctrl`, `Alt` の組み合わせで登録すると，上移動に固定されたり，`Shift` との組み合わせでショートカットが認識されないなどの挙動になっていました．別枠のコマンドとして登録先を新設することでこれらの組み合わせが実現できます．

## 設定ファイルについて

テキストエディタで `tl_walkaround.ini` を編集することで一部挙動を調整できます．詳しくはファイル内のコメントに記述があるためここでは概要のみ紹介．

1. `[skips]`

    無効オブジェクトや非表示レイヤーの中間点やオブジェクト境界を編集点移動の移動先にするかどうかを指定できます．

1. `[scroll]`

    [左へ一定量移動 / 右へ一定量移動](#左へ一定量移動--右へ一定量移動)や[TLスクロール(左) / TLスクロール(右)](#tlスクロール左--tlスクロール右)，[TLスクロール(上) / TLスクロール(下)](#tlスクロール上--tlスクロール下)での移動量を調整できます．

1. `[keyboard]`

    このプラグインでのフレーム移動コマンドに限り，`Shift` キーによる時間範囲選択の影響を無効化します．これにより `Shift` キーとの組み合わせでショートカット登録しても時間範囲選択が起こらなくなります．

## その他

1. InputPipePlugin を導入している場合，現在フレーム移動系のコマンドには `Alt` キーと組み合わせたショートカットキーは控えたほうが無難です．

    フレーム移動の際その AviUtl セッションで未ロードファイルをロードしたときに `Alt` が押されていると，InputPipePlugin の機能で `#.junk` ファイルが意図せず生成されてしまいます．フォルダが `.#junk` ファイルで溢れたり，見覚えのないファイルだと消してしまってプロジェクトファイルが動画ファイルを見つけられなかったりいった事故も考えられます．

    - 関連 issue: https://github.com/amate/InputPipePlugin/issues/7

1. 矢印キーを含めたショートカットキー（`Ctrl+→` など）はタイムラインウィンドウや設定ダイアログにフォーカスがある状態では機能しません．AviUtl メインウィンドウにフォーカスがある場合のみ機能しますが，これを修正するプラグインも公開準備中です．

## 改版履歴

- v1.00 (2024-01-04)

  - 初版．

# ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


#  Credits

##  aviutl_exedit_sdk v1.2

1条項BSD

Copyright (c) 2022
ePi All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
THIS SOFTWARE IS PROVIDED BY ePi “AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL ePi BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#  連絡・バグ報告

- GitHub: https://github.com/sigma-axis
- Twitter: https://twitter.com/sigma_axis
- nicovideo: https://www.nicovideo.jp/user/51492481
- Misskey.io: https://misskey.io/@sigma_axis
