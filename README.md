# CommandLineOption
C++20でコマンドライン引数の解析を簡単に行う

## 利用イメージ
```c++
#include "CommandLineOption.hpp"

int main(int argc, const char* argv[]) {
    option::CommandLineOption clo;
    clo.add_options()
        // --helpというlong option
        .l("help", "ヘルプ")
        // --help=<std::string>というlong option
        .l("help=", option::Value<std::string>(), "何かしらの対象についてのヘルプ")
        // --versionというlong option
        .l("version", "バージョン情報")
        // -o <std::string>というoption(デフォルト引数は"out.txt")
        .o("o", option::Value<std::string>("out.txt").name("out"), "出力ファイル名")
        // --k=<int>...というカンマ区切りでいくらでも0より大きい引数を受け取ることができるlong option
        .l("k", option::Value<int>().unlimited().constraint([](int i) { return 0 < i; }).name("param-k"), "何かしらのパラメータk")
        // 実行するコマンドを受け取る名前なしオプション
        .u(option::Value<std::string>().unlimited().name("command"), "実行するコマンド。詳細はヘルプを参照");

    // 引数がないとき説明を表示
    if (argc == 1) {
        std::cout << "Options:" << std::endl;
        std::cout << clo.description() << std::endl;
        return 0;
    }

    const option::OptionMap& map = clo.map();
    try {
        clo.parse(argc - 1, &argv[1]);
    }
    catch (std::runtime_error& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return 1;
    }

    if (auto temp = map.luse("help="); temp) {
        std::string str = temp.as<std::string>();
        std::cout << "[" << str << "]についてのヘルプ" << std::endl;
        return 0;
    }
    // ヘルプの指定をされたら説明を表示
    else if (map.use("help") || argc == 1) {
        std::cout << "Options:" << std::endl;
        std::cout << clo.description() << std::endl;
        return 0;
    }
    else if (map.use("version")) {
        std::cout << "バージョン情報" << std::endl;
        return 0;
    }

    // パラメータkの取得
    std::cout << "パラメータk:" << std::endl;
    if (auto ks = map.use("k"); ks) {
        for (auto k : ks.as<std::vector<int>>())
            std::cout << k << std::endl;
    }

    // 名前なしオプションのチェック
    std::cout << "unnamed option:" << std::endl;
    if (auto unnamed_options = map.unnamed_options(); unnamed_options) {
        for (const auto& unnamed_option : unnamed_options.as<std::vector<std::string>>())
            std::cout << unnamed_option << std::endl;
    }

    // 出力ファイル名
    std::cout << "出力ファイル名:" << map.use("o").as<std::string>() << std::endl;

    return 0;
}
```

## helpの表示
### 入力コマンド
```
[実行ファイル名]
```

### 実行結果
```
Options:
  --help                   ヘルプ
  --help=<arg>             何かしらの対象についてのヘルプ
  --version                バージョン情報
  -o <out>(=out.txt)       出力ファイル名
  --k[ |=]<param-k...>     何かしらのパラメータk
  <command>                実行するコマンド。詳細はヘルプを参照
```

## 各種入力値の取得
### 入力コマンド
```
[実行ファイル名] --k 1 2 3 4 5 -- aaa bbb ccc
```

### 実行結果
```
パラメータk:
1
2
3
4
5
none option:
aaa
bbb
ccc
出力ファイル名:out.txt
```
