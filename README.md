# TLショトカ移動 AviUtl プラグイン

タイムライン上のオブジェクト境界に移動する，いわゆる編集点移動のショートカットコマンドを追加するプラグインです．他にもスクロール操作などタイムラインをキーボード操作で見て回るのに便利なショートカットコマンドを追加します．

追加機能として，タイムラインをホイールクリックなどのマウス操作で近くの編集点や BPM グリッドに移動する操作もできるようになります．[\[詳細\]](#タイムラインをクリックで編集点や-bpm-グリッドへ移動)

[ダウンロードはこちら．](https://github.com/sigma-axis/aviutl_tl_walkaround/releases) [紹介動画．](https://www.nicovideo.jp/watch/sm43284722)

![編集点移動のデモ](https://github.com/sigma-axis/aviutl_tl_walkaround/assets/132639613/74b8bd73-1551-4068-b7bf-ca8ac2fb95b0)

![編集点にマウス操作で移動](https://github.com/sigma-axis/aviutl_tl_walkaround/assets/132639613/3caf3b34-29d4-46dd-b544-89d9203d233b)

![BPMグリッドにマウス操作で移動](https://github.com/sigma-axis/aviutl_tl_walkaround/assets/132639613/e1883ee6-d433-4edd-8285-e03efd800398)

## 動作要件

- AviUtl 1.10 + 拡張編集 0.92

  http://spring-fragrance.mints.ne.jp/aviutl

  - 拡張編集 0.93rc1 等の他バージョンでは動作しません．

- Visual C++ 再頒布可能パッケージ（\[2015/2017/2019/2022\] の x86 対応版が必要）

  https://learn.microsoft.com/ja-jp/cpp/windows/latest-supported-vc-redist

## 導入方法

以下のフォルダのいずれかに `tl_walkaround.auf` と `tl_walkaround.ini` をコピーしてください．

1. `aviutl.exe` のあるフォルダ
1. (1) のフォルダにある `plugins` フォルダ
1. (2) のフォルダにある任意のフォルダ

## 使い方

以下のコマンドが追加されます．

![編集→TLショトカ移動のメニュー](https://github.com/sigma-axis/aviutl_tl_walkaround/assets/132639613/590d3623-03dc-44d7-b46c-0ec6de5eda4d)

AviUtlのメニューから「ファイル」→「環境設定」→「ショートカットキーの設定」とたどって，これらのコマンドを好みのキーに割り当ててください．

また初期状態では無効化されていますが，タイムライン上をホイールクリックなどのマウス操作で近くの編集点や BPM グリッド上の線に移動する機能もあります．[\[詳細\]](#タイムラインをクリックで編集点や-bpm-グリッドへ移動)

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

### 左の小節線に移動(BPM) / 右の小節線に移動(BPM) / 左の拍数位置に移動(BPM) / 右の拍数位置に移動(BPM)

BPM グリッドに沿って現在フレームを移動します．現在位置から最も近い左(右)にある，小節線や拍数線に移動します．

### 左の1/4拍位置に移動(BPM) / 右の1/4拍位置に移動(BPM) / 左の1/N拍位置に移動(BPM) / 右の1/N拍位置に移動(BPM)

上記のコマンド「[左の小節線に移動(BPM) / 右の小節線に移動(BPM) / 左の拍数位置に移動(BPM) / 右の拍数位置に移動(BPM)](#左の小節線に移動bpm--右の小節線に移動bpm--左の拍数位置に移動bpm--右の拍数位置に移動bpm)」よりも細かい単位で移動します．

- 「左(右)の1/4拍位置に移動(BPM)」は 1/4 拍単位で移動します．
- 「左(右)の1/N拍位置に移動(BPM)」の `N` は[設定ファイルで指定](#設定ファイルについて)できます．
  - `N` の初期値は 3 で，1 拍の 1/3 だけ移動します．この場合 3 連符の曲に合いやすくなります．

### グリッドを左に移動(BPM) / グリッドを右に移動(BPM)

BPM グリッドの基準フレームを移動して，グリッドを左(右)に1フレームだけ移動します．

### 最寄りの小節線を現在位置に(BPM)

BPM グリッドの基準フレームを移動して，一番近い小節線を現在フレームの位置に合わせます．

### 基準フレームを現在位置に(BPM)

BPM グリッドの基準フレームを，現在フレームの位置に設定します．

> [!TIP]
> 「[最寄りの小節線を現在位置に(BPM)](#最寄りの小節線を現在位置にbpm)」と類似の挙動ですが，BPM グリッド線の位置の計算時の，小数点以下のフレーム数を整数で近似する部分に影響します．
>
> リタルダンドやフェルマータ直後などのグリッド位置調整には「最寄りの小節線を現在位置に(BPM)」，新しい曲の開始時には「基準フレームを現在位置に(BPM)」と使い分けると誤差の蓄積を抑えやすくなります．
>
> 誤差は 1 か所当たり平均 1 フレーム未満です．なので過大評価は厳禁ですが，基準フレーム前後では 1 拍のフレーム数が小数点以下切り上げで計算されるため，「基準フレームを現在位置に(BPM)」を繰り返し継ぎ足ししていくと誤差が蓄積しやすくなります．

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

### タイムラインをクリックで編集点や BPM グリッドへ移動

「`Shift`+右クリック」「ホイールクリック」「X1ボタン」「X2ボタン」のいずれかでタイムラインをクリックすると，クリックした点の近くにある編集点，あるいは BPM グリッド上の線に現在フレームが移動します．

- 初期状態ではボタン割り当てをしていないため無効化されています．利用するには[設定ファイルを編集](#設定ファイルについて)して有効化してください．

- 「`Shift`+右クリック」「ホイールクリック」「X1ボタン」「X2ボタン」の4つのボタンから[設定ファイルを編集して](#設定ファイルについて)割り当ててください．
  - [編集点への移動](#編集点へ移動)と [BPM グリッドへの移動](#bpm-グリッドへ移動)，それぞれ独立にマウスボタンを選べます．
  - 右クリック割り当て時の `Shift` は必須です．

#### 編集点へ移動

クリックしたレイヤー上にあるオブジェクト境界や中間点に止まります．他レイヤーのオブジェクト配置は影響しません．

- [設定](#設定ファイルについて)によっては，`Shift` キーや `Ctrl` キーの押下状態で中間点や無効化オブジェクトを無視させることもできます．
  - `Alt` キーとの組み合わせはできません．[InputPipePlugin に干渉してしまう](#その他)のが理由です．

#### BPM グリッドへ移動

クリックした点に最も近い小節線や拍数線に止まります．

- [設定](#設定ファイルについて)によって，`Shift` キーや `Ctrl` キーの押下状態で小節線，拍数線，1/4 拍単位，1/N 拍単位のどれに止まるかを調整できます．
  - 「1/N 拍」の `N` は設定ファイルの `[bpm_grid]` 以下で指定した値になります．
  - 編集点移動の場合と同様，`Alt` キーとの組み合わせはできません．

## 設定ファイルについて

テキストエディタで `tl_walkaround.ini` を編集することで一部挙動を調整できます．詳しくはファイル内のコメントに記述があるためここでは概要のみ紹介．

1. `[skips]`

   無効オブジェクトや非表示レイヤーの中間点やオブジェクト境界を編集点移動の移動先にするかどうかを指定できます．

1. `[scroll]`

   [左へ一定量移動 / 右へ一定量移動](#左へ一定量移動--右へ一定量移動)や[TLスクロール(左) / TLスクロール(右)](#tlスクロール左--tlスクロール右)，[TLスクロール(上) / TLスクロール(下)](#tlスクロール上--tlスクロール下)での移動量を調整できます．

1. `[keyboard]`

   このプラグインでのフレーム移動コマンドに限り，`Shift` キーによる時間範囲選択の影響を無効化します．これにより `Shift` キーとの組み合わせでショートカット登録しても時間範囲選択が起こらなくなり，`Shift` なしの場合と同じ挙動になります．

   - [タイムラインをクリックで編集点や BPM グリッドへ移動](#タイムラインをクリックで編集点や-bpm-グリッドへ移動)の機能には影響しません．そちらは `[mouse]` 以下に別枠で設定があります．

1. `[bpm_grid]`

   [左の1/N拍位置に移動(BPM) / 右の1/N拍位置に移動(BPM)](#左の14拍位置に移動bpm--右の14拍位置に移動bpm--左の1n拍位置に移動bpm--右の1n拍位置に移動bpm) のコマンドでの `N` の値を指定できます．

1. `[mouse]`

   [タイムラインをクリックで編集点や BPM グリッドへ移動](#タイムラインをクリックで編集点や-bpm-グリッドへ移動)の設定です．

## その他

1. [InputPipePlugin](https://github.com/amate/InputPipePlugin) を導入している場合，現在フレーム移動系のコマンドには `Alt` キーと組み合わせたショートカットキーは控えたほうが無難です．

    フレーム移動の際その AviUtl セッションで未ロードファイルをロードしたときに `Alt` が押されていると，InputPipePlugin の機能で `.#junk` ファイルが意図せず生成されてしまいます．フォルダが `.#junk` ファイルで溢れたり，見覚えのないファイルだと消してしまってプロジェクトファイルが動画ファイルを見つけられなかったりいった事故も考えられます．

    - 関連 issue: https://github.com/amate/InputPipePlugin/issues/7

1. 矢印キーを含めたショートカットキー（`Ctrl + →` など）はタイムラインウィンドウや設定ダイアログにフォーカスがある状態では機能しません．AviUtl メインウィンドウにフォーカスがある場合のみ機能しますが，これを[修正するプラグインも公開](https://github.com/sigma-axis/aviutl_allow_arrow)しました．

## 改版履歴

- **v1.20** (2024-04-??)

  - タイムライン上をマウスボタンクリックで近くの編集点や BPM グリッドの拍数線に移動する機能を追加．
    - 次のマウスボタンに割り当てられます:
      1. `Shift`+右クリック
      1. ホイールクリック
      1. X1 ボタン
      1. X2 ボタン
    - v1.10 以前からの更新でこの機能を利用するには，新しくダウンロードした `.ini` ファイル内の `[mouse]` セクション以下を，現在使用中の `.ini` ファイル末尾に追記してください．
    - 初期状態では無効化されています．詳しいパラメタの設定方法は `.ini` ファイル内のコメントを参照してください．

- **v1.10** (2024-03-13)

  - BPM グリッドに沿った移動コマンドを追加．

  - BPM グリッドの基準フレームを操作するコマンドを追加．

- **v1.02** (2024-02-28)

  - 初期化周りを少し改善．

- **v1.01** (2024-01-11)

  - タイムラインスクロールバーのハンドル取得方法を変更．

  - 冗長な null チェックを削除．

- **v1.00** (2024-01-04)

  - 初版．

## ライセンス

このプログラムの利用・改変・再頒布等に関しては MIT ライセンスに従うものとします．

---

The MIT License (MIT)

Copyright (C) 2024 sigma-axis

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

https://mit-license.org/


#  Credits

##  aviutl_exedit_sdk

https://github.com/ePi5131/aviutl_exedit_sdk

---

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
- Bluesky: https://bsky.app/profile/sigma-axis.bsky.social
