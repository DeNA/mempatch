mempatch
========

Android端末で動作しているアプリケーションのメモリデータを書き換えるツールです

注意
---

本プログラムは、脆弱性診断での利用や開発におけるサポートとしての利用を想定したものであるため**リリースされたアプリに対して実行しないでください**


ビルド方法
--------------

ビルドにAndroid NDKが必要となるので、あらかじめインストールしておきます。
インストールが完了したら、Android NDKのndk-buildまたはCMakeでビルドできます。

### ndk-buildを使用してビルドする

```
$ ~/Library/Android/sdk/ndk/VERSION/ndk-build
[arm64-v8a] Install        : mempatch => libs/arm64-v8a/mempatch
[arm64-v8a] Install        : victim_ascii => libs/arm64-v8a/victim_ascii
[arm64-v8a] Install        : victim_int => libs/arm64-v8a/victim_int
.. 略 ...
```

上記コマンドが成功すると、libs/arm64-v8a/mempatchという実行ファイルが生成されます。

### CMakeを使用してビルドする

buildディレクトリを作り、buildディレクトリの中でcmakeコマンドを以下のように実行します(Windows以外)。

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_TOOLCHAIN_FILE=/path/to/ndk/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a ..
$ make -j8
```

Windowsの場合、[Ninja](https://github.com/ninja-build/ninja)をインストールしてビルドします。

```
$ mkdir build
$ cd build
$ cmake -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=/path/to/ndk/build/cmake/android.toolchain.cmake -DANDROID_ABI=arm64-v8a ..
$ ninja
```

上記コマンドが成功すると、build/jni/mempatchという実行ファイルが生成されます。

Androidへの組み込み
-------------------

生成されたmempatchをadbコマンドでアプリのディレクトリにコピーしてください。

```
$ adb push libs/arm64-v8a/mempatch /data/local/tmp/mempatch
3381 KB/s (259644 bytes in 0.074s)
$ adb shell
$ run-as com.example.Sample
$ pwd
/data/data/com.example.Sample
$ cp /data/local/tmp/mempatch ./
```

root権限が無い場合、ターゲットアプリがdebugモードになっていないと使えません。
apktoolなどを使ってAndroidManifest.xmlでandroid:debuggable="true"としてインストールしてください。
cpが利用できない場合は、catとchmodを利用してください。

実行
----

ターゲットアプリを実行後、adb shellでAndroidのshellを立ち上げ、run-asでターゲットアプリのユーザになります。
次にターゲットアプリのプロセスIDを指定して、mempatchを起動します。
ターゲットアプリのパッケージ名・プロセスIDはps, pmコマンドなどで調べてください。

```
$ adb shell
$ run-as com.example.Sample

$ ps | grep com.example.Sample
u0_a195   31543 223   711240 136272 ffffffff 40183ab0 S com.example.Sample
$ /data/local/tmp/mempatch -w -p 31543
Without Ptrace Mode

Please Input Command
```

mempatchを起動した後はattach, detach, lookup, filterなどを使用して改竄したいデータのアドレスを見つけ、
changeで改竄を行います。
下記の実行例を参考にしてください。

```
> # 改竄したいデータがUI上で見える場所までアプリを動かす
> # UI上から確認可能な値が10000
> lookup int 10000
Starting memory patching... (mode: lookup)
Process Time: 1194 ms
Memory: 188.22 MB
Range Size: 506
Found Address: 32

Please Input Command
>
> # 何らかの方法で値を操作する
> # その結果、UI上から確認可能な値が9964になった
> filter int 9964
Starting memory patching... (mode: filter)
Process Time: 62 ms
Memory: 188.22 MB
Range Size: 506
Found Address: 1

Please Input Command
>
> # アドレスが十分に絞り込めたので改ざんをする
> change int 1
Starting memory patching... (mode: change)
Change: ec260000(9964) -> 01000000(1) ([anon:libc_malloc])
Dump: dc71ab88-dc71abac (36 byte, [anon:libc_malloc])
  dc71ab88 70ab 58de 70ab 58de 74ab 58de 00f1 4dde  p.X.p.X.t.X...M.
  dc71ab98 0100 0000 c201 0000 c201 0000 d900 0000  ................
  dc71aba8 0000 0000                                ....
Replace Count: 1 / 1
Process Time: 115 ms
Memory: 188.22 MB
Range Size: 506
Found Address: 1

Please Input Command
>
```

詳細なコマンドに関しては`mempatch -h`をするかソースコードを参照してください。

Permission denied(13) : Fail wait pid=5994と言われたら
------
root化端末でメモリ改ざんしようとして上記のエラーが出たらSELinuxのせいです。

```
>attach
...
  Attach 6907 thread
Permission denied(13) : Fail wait pid=5994
argument is invalid or failed to process
```

SELinuxを無効にすれば動きます。

```
root@angler:/data/local/tmp # setenforce 0
```

