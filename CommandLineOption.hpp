#pragma once

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <format>

namespace option {

    /// <summary>
    /// 型名を示す文字列を取得するためのメタ関数
    /// </summary>
    template <class T> struct type_name { static constexpr std::string_view value = "Unknwon"; };
#define DECLARE_TYPE_NAME(name)\
    template <> struct type_name<name> { static constexpr std::string_view value = #name; };
    DECLARE_TYPE_NAME(std::string)
    DECLARE_TYPE_NAME(int)
    DECLARE_TYPE_NAME(long)
    DECLARE_TYPE_NAME(long long)
    DECLARE_TYPE_NAME(unsigned long long)
    DECLARE_TYPE_NAME(float)
    DECLARE_TYPE_NAME(double)
    DECLARE_TYPE_NAME(long double)
#undef DECLARE_TYPE_NAME

    /// <summary>
    /// optionなどの基底
    /// </summary>
    class OptionBase {
    protected:
        /// <summary>
        /// オプション名
        /// </summary>
        std::string _name;
        /// <summary>
        /// オプションの説明
        /// </summary>
        std::string _description;
        /// <summary>
        /// オプションが利用されているときにtrue
        /// </summary>
        bool _use = false;

    public:
        OptionBase() = delete;
        OptionBase(const std::string& name, const std::string& description) : _name(name), _description(description) {
            if (name[0] == '-') {
                throw std::invalid_argument("option名の1文字目は'-'にすることはできません");
            }
            if (name.find('=') != std::string::npos) {
                throw std::invalid_argument("optionに等号を含めることはできません");
            }
            if (name.find(' ') != std::string::npos) {
                throw std::invalid_argument("optionに空白スペースを含めることはできません");
            }
        }
        virtual ~OptionBase() {}

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        virtual OptionBase* clone() const = 0;

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="offset">コマンドライン引数のオフセット</param>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <returns>解析を実行したときにtrue</returns>
        virtual bool parse(int& offset, int& argc, const char* argv[]) = 0;

        /// <summary>
        /// オプション名の取得
        /// </summary>
        /// <returns>オプション名</returns>
        const std::string& name() const noexcept { return this->_name; }

        /// <summary>
        /// 接頭辞付きのオプション名の取得
        /// </summary>
        /// <returns>接頭辞付きのオプション名</returns>
        virtual std::string full_name() const { return this->name(); }

        /// <summary>
        /// メモリの確保なしにオプション名がマッチするかの判定
        /// </summary>
        /// <returns>接頭辞付きのオプション名</returns>
        virtual bool match_name(std::string_view str) const { return this->_name == str; }

        /// <summary>
        /// オプションの説明の取得
        /// </summary>
        /// <returns>オプションの説明</returns>
        const std::string& description() const noexcept { return this->_description; }

        /// <summary>
        /// オプション名についての説明
        /// </summary>
        /// <returns></returns>
        virtual std::string name_description() const { return this->full_name(); }

        /// <summary>
        /// オプションが利用されているかの取得
        /// </summary>
        /// <returns></returns>
        bool use() const noexcept { return this->_use; }

        /// <summary>
        /// コマンドライン解析前の状態へ初期化
        /// </summary>
        virtual void init() {}

        /// <summary>
        /// 与えられた引数のチェック
        /// </summary>
        virtual void validate() {}

        /// <summary>
        /// 引数がハイフンのみで構成されるかの判定
        /// </summary>
        /// <param name="str">判定対象の文字列</param>
        /// <returns></returns>
        static constexpr bool is_dash(const char* str) noexcept {
            if (str == nullptr) return false;
            for (const char* str2 = str; *str2 != '\0'; ++str2) {
                if (*str2 != '-') {
                    return false;
                }
            }
            return *str != '\0';
        }
    };

    /// <summary>
    /// 引数付きoptionもしくはlong optionの基底
    /// </summary>
    class OptionHasValueBase {
    public:
        /// <summary>
         /// optionの入力パターン
         /// </summary>
        struct ARG_PATTERN {
            /// <summary>
            /// なし
            /// </summary>
            static constexpr std::size_t NONE = 0;
            /// <summary>
            /// --a=1のような等号による引数から記述が始まることを示すフラグ
            /// </summary>
            static constexpr std::size_t ASSIGN = 0b1;
            /// <summary>
            /// --a 1のようなスペースによる引数から記述が始まることを示すフラグ
            /// </summary>
            static constexpr std::size_t SPACE = 0b10;
        };
    protected:
        /// <summary>
         /// 引数の記載パターン
         /// </summary>
        std::size_t _arg_pattern = ARG_PATTERN::NONE;

        /// <summary>
        /// 引数の記載パターンに関する説明の取得
        /// </summary>
        /// <returns></returns>
        std::string arg_pattern_description() const {
            switch (this->_arg_pattern) {
            case ARG_PATTERN::NONE:
                return "";
            case ARG_PATTERN::ASSIGN:
                return "=";
            case ARG_PATTERN::SPACE:
                return " ";
            default:
                if (this->_arg_pattern == (ARG_PATTERN::SPACE | ARG_PATTERN::ASSIGN)) {
                    return "[ |=]";
                }
            }
            throw std::logic_error("ここに来ることはない");
        }

    public:
        OptionHasValueBase(std::size_t arg_pattern) : _arg_pattern(arg_pattern) {}

        /// <summary>
        /// 引数の入力パターン
        /// </summary>
        /// <returns></returns>
        std::size_t arg_pattern() const noexcept { return this->_arg_pattern; }
    };

    /// <summary>
    /// option
    /// </summary>
    class Option : public OptionBase {
    public:
        using OptionBase::OptionBase;
        virtual ~Option() {}

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        virtual OptionBase* clone() const { return new Option(*this); }

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="offset">コマンドライン引数のオフセット</param>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <returns>解析を実行したときにtrue</returns>
        virtual bool parse(int& offset, int& argc, const char* argv[]) {
            if (this->match_name(argv[offset])) {
                this->_use = true;
                ++offset;
                return true;
            }
            return false;
        }

        /// <summary>
        /// 接頭辞付きのオプション名の取得
        /// </summary>
        /// <returns>接頭辞付きのオプション名</returns>
        virtual std::string full_name() const { return "-" + this->name(); }

        /// <summary>
        /// メモリの確保なしにオプション名がマッチするかの判定
        /// </summary>
        /// <returns>接頭辞付きのオプション名</returns>
        virtual bool match_name(std::string_view str) const {
            return str.size() >= 2 && str[0] == '-' && this->_name == str.substr(1);
        }

        /// <summary>
        /// optionであることの判定
        /// </summary>
        /// <param name="str">判定対象の文字列</param>
        /// <returns>optionならtrue</returns>
        static constexpr bool is_option(const char* str) {
            if (str == nullptr) return false;
            return *str == '-' && *(str + 1) != '-' && *(str + 1) != '\0';
        }
    };

    /// <summary>
    /// long option
    /// </summary>
    class LongOption : public OptionBase {
    public:
        using OptionBase::OptionBase;
        virtual ~LongOption() {}

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        virtual OptionBase* clone() const { return new LongOption(*this); }

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="offset">コマンドライン引数のオフセット</param>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <returns>解析を実行したときにtrue</returns>
        virtual bool parse(int& offset, int& argc, const char* argv[]) {
            if (this->match_name(argv[offset])) {
                this->_use = true;
                ++offset;
                return true;
            }
            return false;
        }

        /// <summary>
        /// 接頭辞付きのオプション名の取得
        /// </summary>
        /// <returns>接頭辞付きのオプション名</returns>
        virtual std::string full_name() const { return "--" + this->name(); }

        /// <summary>
        /// メモリの確保なしにオプション名がマッチするかの判定
        /// </summary>
        /// <returns>接頭辞付きのオプション名</returns>
        virtual bool match_name(std::string_view str) const {
            return str.size() >= 3 && str[0] == '-' && str[1] == '-' && this->_name == str.substr(2);
        }

        /// <summary>
        /// long optionであることの判定
        /// </summary>
        /// <param name="str">判定対象の文字列</param>
        /// <returns>long optionならtrue</returns>
        static constexpr bool is_long_option(const char* str) {
            if (str == nullptr) return false;
            return *str == '-' && *(str + 1) == '-' && *(str + 2) != '-' && *(str + 2) != '\0';
        }
    };

    /// <summary>
    /// optionに与える引数
    /// </summary>
    /// <typeparam name="T">引数の型</typeparam>
    template <class T>
    class Value {
        template <class U> friend class OptionValue;

        /// <summary>
        /// デフォルト引数
        /// </summary>
        std::vector<T> _default_value;
        /// <summary>
        /// 引数の制約条件
        /// </summary>
        std::function<bool(T)> _constraint;
        /// <summary>
        /// 引数の数の上限
        /// </summary>
        std::size_t _limit = 1;
        /// <summary>
        /// 引数の表示名(helpで<_name...[1-_limit]>のように表示される)
        /// </summary>
        std::string _name = "arg";
        /// <summary>
        /// 必須項目となる件数
        /// </summary>
        std::size_t _required = 0;

    public:
        Value() {}
        Value(const T& x) : _default_value(1, x) {}
        Value(std::initializer_list<T> x) : _default_value(x) {}

        /// <summary>
        /// 引数の制約条件の設定
        /// </summary>
        /// <typeparam name="F">引数の制約式の型</typeparam>
        /// <param name="f">引数の制約式</param>
        /// <returns></returns>
        template <class F>
        Value& constraint(F f) {
            // デフォルト引数が制約条件を満たしているかのチェック
            for (const auto& value : this->_default_value) {
                if (!f(value)) {
                    throw std::logic_error("デフォルト引数が満たさない制約条件を設定することはできません");
                }
            }
            this->_constraint = f;
            return *this;
        }

        /// <summary>
        /// 引数の数の上限の設定
        /// </summary>
        /// <param name="l">引数の数の上限</param>
        /// <returns></returns>
        Value& limit(std::size_t l) {
            if (l == 0) throw std::logic_error("保持する引数の数は0に設定することはできません");
            if (l < this->_default_value.size()) throw std::logic_error("デフォルト引数の数が引数の数の制限を超過しています");
            this->_limit = l;
            return *this;
        }

        /// <summary>
        /// 引数の数の上限をなしにする
        /// </summary>
        /// <returns></returns>
        Value& unlimited() {
            this->_limit = std::numeric_limits<std::size_t>::max();
            return *this;
        }

        /// <summary>
        /// 引数の表示名の設定
        /// </summary>
        /// <param name="n">引数の表示名</param>
        /// <returns></returns>
        Value& name(const std::string& n) {
            this->_name = n;
            return *this;
        }

        /// <summary>
        /// 必須項目として設定する
        /// </summary>
        /// <param name="n">必須項目となる件数</param>
        /// <returns></returns>
        Value& required(std::size_t n = std::numeric_limits<std::size_t>::max()) {
            this->_required = n;
            return *this;
        }

        /// <summary>
        /// デフォルト引数を持つかの判定
        /// </summary>
        /// <returns>デフォルト引数を持つ場合にtrue</returns>
        bool has_default() const { return !this->_default_value.empty(); }

        /// <summary>
        /// 文字列をValueとして利用可能な型に変換する
        /// </summary>
        /// <param name="str_view">変換対象の文字列</param>
        /// <returns>変換結果</returns>
        T transform(std::string_view str_view) {
            std::string str(str_view.data(), str_view.size());
            if constexpr (!std::is_same_v<T, std::string>) {
                std::istringstream stream(str);
                T result;
                stream >> result;
                // 変換できなかった場合は例外を投げる
                if (!(bool(stream) && (stream.eof() || stream.get() == std::char_traits<char>::eof()))) {
                    throw std::runtime_error(std::format("{0} は型 {1} に変換することはできません", str, type_name<T>::value));
                }

                return result;
            }
            else {
                return str;
            }
        }
    };

    /// <summary>
    /// option引数に関する型
    /// </summary>
    template <class T>
    class OptionValue {
        /// <summary>
        /// 引数の設定
        /// </summary>
        Value<T> _value_info;
        /// <summary>
        /// optionに対する引数
        /// </summary>
        std::vector<T> _value;

    protected:
        /// <summary>
        /// option引数に関する説明の取得
        /// </summary>
        /// <returns></returns>
        std::string option_value_description() const {
            // 引数の形式の取得
            std::string arg = "<";
            arg += this->_value_info._name;
            std::size_t limit = this->_value_info._limit;
            if (limit == std::numeric_limits<std::size_t>::max()) {
                arg += "...";
            }
            else if (limit > 1) {
                arg += "...[1-" + std::to_string(limit) + "]";
            }
            arg += ">";
            if (this->_value_info.has_default()) {
                // デフォルト引数をカンマつなぎにする
                std::stringstream stream;
                auto& default_value = this->_value_info._default_value;
                stream << default_value[0];
                for (std::size_t i = 1; i < default_value.size(); ++i) {
                    stream << "," << default_value[i];
                }
                arg += "(=" + stream.str() + ")";
            }
            return arg;
        }

        /// <summary>
        /// 引数の追加
        /// </summary>
        /// <param name="val">引数を示す文字列</param>
        void append(std::string_view val) {
            this->_value.push_back(this->_value_info.transform(val));
        }

        /// <summary>
        /// 引数の検査
        /// </summary>
        void validateArg() {
            // チェック対象の引数
            const auto& targets = this->_value.size() != 0 ? this->_value : this->_value_info._default_value;

            // 引数の数のチェック
            if (targets.size() > this->_value_info._limit) {
                throw std::runtime_error("引数の数が多すぎます");
            }
            if (this->_value_info._limit == std::numeric_limits<std::size_t>::max() && this->_value_info._required == std::numeric_limits<std::size_t>::max()) {
                // 任意の数の引数を取ることができる場合かつデフォルトの必須の場合は1つのみ必須とする
                if (this->_value_info._default_value.size() == 0 && targets.size() == 0) {
                    throw std::runtime_error("引数の数が少なすぎます");
                }
            }
            else {
                std::size_t required = std::min(this->_value_info._limit, this->_value_info._required);
                if (targets.size() < required) {
                    throw std::runtime_error("引数の数が少なすぎます");
                }
            }

            // 引数の制約条件のチェック
            if (this->_value_info._constraint) {
                for (const auto& target : targets) {
                    if (!this->_value_info._constraint(target)) {
                        throw std::runtime_error(std::format("{0} は制約条件を満たしていません", target));
                    }
                }
            }
        }

        /// <summary>
        /// 設定された引数のクリア
        /// </summary>
        void clearArg() {
            this->_value.clear();
        }

        /// <summary>
        /// 引数の数の取得
        /// </summary>
        std::size_t argNum() {
            return this->_value.size();
        }

        /// <summary>
        /// 引数の数の上限の取得
        /// </summary>
        std::size_t argLimit() {
            return this->_value_info._limit;
        }

    public:
        OptionValue(const Value<T>& value_info) : _value_info(value_info) {}

        /// <summary>
        /// 保持している値の取得
        /// </summary>
        /// <typeparam name="U">std::vector<T></typeparam>
        /// <typeparam name="type"></typeparam>
        /// <returns></returns>
        template <class U, typename std::enable_if<!std::is_same<T, U>::value, std::nullptr_t>::type = nullptr>
        U as() const {
            if (this->_value.size() == 0) {
                // デフォルト引数の検査
                if (this->_value_info.has_default()) {
                    return this->_value_info._default_value;
                }
                throw std::runtime_error("引数が設定されていません");
            }
            return  this->_value;
        };

        /// <summary>
        /// 保持している値の取得
        /// </summary>
        /// <typeparam name="U">T</typeparam>
        /// <typeparam name="type"></typeparam>
        /// <returns></returns>
        template <class U, typename std::enable_if<std::is_same<T, U>::value, std::nullptr_t>::type = nullptr>
        U as() const {
            if (this->_value.size() == 0) {
                // デフォルト引数の検査
                if (this->_value_info.has_default()) {
                    return this->_value_info._default_value[0];
                }
                throw std::runtime_error("引数が設定されていません");
            }
            return this->_value[0];
        };
    };

    /// <summary>
    /// 引数付きのoption
    /// </summary>
    template <class T>
    class OptionHasValue : public Option, public OptionValue<T>, public OptionHasValueBase {
    public:
        OptionHasValue(const Value<T>& value_info, const std::string& name, const std::string& description) : OptionValue<T>(value_info), Option(name, description), OptionHasValueBase(OptionHasValueBase::ARG_PATTERN::SPACE) {
            if (value_info.has_default()) {
                this->_use = true;
            }
        }

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        virtual OptionBase* clone() const { return new OptionHasValue(*this); }

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="offset">コマンドライン引数のオフセット</param>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <returns>解析を実行したときにtrue</returns>
        virtual bool parse(int& offset, int& argc, const char* argv[]) {
            if (this->match_name(argv[offset])) {
                int offset2 = offset + 1;

                std::size_t i = this->argNum();
                std::size_t limit = std::min(i + 1, this->argLimit());
                if (limit > 0 && this->argNum() == limit) {
                    throw std::runtime_error(std::format("option {0} でこれ以上の引数を指定することはできません", this->full_name()));
                }

                for (std::size_t j = i; j < limit && offset2 < argc; ++j, ++offset2) {
                    const char* token = argv[offset2];
                    if (Option::is_option(token) || LongOption::is_long_option(token)) {
                        // 次のトークンがoptionかlong optionの時は中断
                        break;
                    }
                    if (OptionBase::is_dash(token)) {
                        // 先読みを行ってそれが「-」から始まるならそれをパラメータとして扱う
                        if (++offset2 < argc && argv[offset2][0] == '-') {
                            token = argv[offset2];
                        }
                        else {
                            // long optionの解析を中断
                            break;
                        }
                    }
                    try {
                        // 引数に追加
                        this->append(token);
                    }
                    catch (const std::runtime_error& e) {
                        throw std::runtime_error(std::format("option {0} に対する引数 {1}", this->full_name(), e.what()));
                    }
                }

                if (i == this->argNum() && limit > 0) {
                    throw std::runtime_error(std::format("option {0} には引数を指定する必要があります", this->full_name()));
                }
                this->_use = true;
                offset = offset2;
                return true;
            }
            return false;
        }

        /// <summary>
        /// オプション名についての説明
        /// </summary>
        /// <returns></returns>
        virtual std::string name_description() const {
            return this->full_name() + this->arg_pattern_description() + this->option_value_description();
        }

        /// <summary>
        /// コマンドライン解析前の状態へ初期化
        /// </summary>
        virtual void init() {
            this->clearArg();
        }

        /// <summary>
        /// 与えられた引数のチェック
        /// </summary>
        virtual void validate() {
            this->validateArg();
        }
    };

    /// <summary>
    /// 引数付きのlong option
    /// </summary>
    template <class T>
    class LongOptionHasValue : public LongOption, public OptionValue<T>, public OptionHasValueBase {
    public:
        LongOptionHasValue(const Value<T>& value_info, const std::string& name, const std::string& description, std::size_t arg_pattern) : OptionValue<T>(value_info), LongOption(name, description), OptionHasValueBase(arg_pattern) {
            if (value_info.has_default()) {
                this->_use = true;
            }
        }

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        virtual OptionBase* clone() const { return new LongOptionHasValue(*this); }

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="offset">コマンドライン引数のオフセット</param>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <returns>解析を実行したときにtrue</returns>
        virtual bool parse(int& offset, int& argc, const char* argv[]) {
            const auto str = std::string_view{ argv[offset] };
            std::size_t i = str.find('=');
            if (this->match_name(str.substr(0, i))) {
                int offset2 = offset + 1;
                std::size_t limit = this->argLimit();

                if (i != std::string::npos) {
                    if ((this->_arg_pattern & ARG_PATTERN::ASSIGN) != ARG_PATTERN::ASSIGN) {
                        // 解析は不可のためスルー
                        return false;
                    }
                    else {
                        // 「=」による指定では1つのみ指定可能
                        if (limit > 0 && this->argNum() == limit) {
                            throw std::runtime_error(std::format("option {0} でこれ以上の引数を指定することはできません", this->full_name()));
                        }
                        try {
                            // 引数に追加
                            this->append(str.substr(i + 1));
                        }
                        catch (const std::runtime_error& e) {
                            throw std::runtime_error(std::format("option {0} に対する引数 {1}", this->full_name(), e.what()));
                        }

                        this->_use = true;
                        offset = offset2;
                        return true;
                    }
                }
                else {
                    if ((this->_arg_pattern & ARG_PATTERN::SPACE) != ARG_PATTERN::SPACE) {
                        // 解析は不可のためスルー
                        return false;
                    }
                }

                std::size_t j = this->argNum();
                if (limit > 0 && j == limit) {
                    throw std::runtime_error(std::format("option {0} でこれ以上の引数を指定することはできません", this->full_name()));
                }

                for (; j < limit && offset2 < argc; ++j, ++offset2) {
                    const char* token = argv[offset2];
                    if (Option::is_option(token) || LongOption::is_long_option(token)) {
                        // 次のトークンがoptionかlong optionの時は中断
                        break;
                    }
                    if (OptionBase::is_dash(token)) {
                        // 先読みを行ってそれが「-」から始まるならそれをパラメータとして扱う
                        if (++offset2 < argc && argv[offset2][0] == '-') {
                            token = argv[offset2];
                        }
                        else {
                            // long optionの解析を中断
                            break;
                        }
                    }
                    try {
                        // 引数に追加
                        this->append(token);
                    }
                    catch (const std::runtime_error& e) {
                        throw std::runtime_error(std::format("option {0} に対する引数 {1}", this->full_name(), e.what()));
                    }
                }

                if (j == 0 && limit > 0) {
                    throw std::runtime_error(std::format("option {0} には引数を指定する必要があります", this->full_name()));
                }
                this->_use = true;
                offset = offset2;
                return true;
            }
            return false;
        }

        /// <summary>
        /// オプション名についての説明
        /// </summary>
        /// <returns></returns>
        virtual std::string name_description() const {
            return this->full_name() + this->arg_pattern_description() + this->option_value_description();
        }

        /// <summary>
        /// コマンドライン解析前の状態へ初期化
        /// </summary>
        virtual void init() {
            this->clearArg();
        }

        /// <summary>
        /// 与えられた引数のチェック
        /// </summary>
        virtual void validate() {
            this->validateArg();
        }
    };

    /// <summary>
    /// 名前なしオプション
    /// </summary>
    template <class T>
    class UnnamedOption : public OptionBase, public OptionValue<T> {
        friend class AddOptions;
        /// <summary>
        /// trueなら後続の解析を中断する
        /// </summary>
        bool _pause = false;

    public:
        UnnamedOption(const Value<T>& value_info, const std::string& description) : OptionValue<T>(value_info), OptionBase("", description) {}

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        virtual OptionBase* clone() const { return new UnnamedOption(*this); }

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="offset">コマンドライン引数のオフセット</param>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <returns>解析を実行したときにtrue</returns>
        virtual bool parse(int& offset, int& argc, const char* argv[]) {
            if (this->argNum() < this->argLimit()) {
                try {
                    // 引数に追加
                    this->append(argv[offset]);
                }
                catch (const std::runtime_error& e) {
                    throw std::runtime_error(std::format("option {0} に対する引数 {1}", this->full_name(), e.what()));
                }
                ++offset;
                this->_use = true;
                if (this->argNum() == this->argLimit() && this->_pause) {
                    // 中断をする場合はその旨を設定する
                    argc = offset;
                }
                return true;
            }
            return false;
        }

        /// <summary>
        /// オプション名についての説明
        /// </summary>
        /// <returns></returns>
        virtual std::string name_description() const {
            return this->option_value_description();
        }

        /// <summary>
        /// コマンドライン解析前の状態へ初期化
        /// </summary>
        virtual void init() {
            this->clearArg();
        }

        /// <summary>
        /// 与えられた引数のチェック
        /// </summary>
        virtual void validate() {
            this->validateArg();
        }
    };

    /// <summary>
    /// コマンドラインオプションの解析結果を取得するためのクラス
    /// </summary>
    class OptionWrapper {
        std::shared_ptr<OptionBase> _option;

        // 型Tがvalue_typeをもつならその型に変換
        template <class T, class = void>
        struct to_value_type {
            using type = T;
        };
        template <class T>
        struct to_value_type<T, std::void_t<typename T::value_type>> {
            using type = typename T::value_type;
        };
        // std::stringは例外
        template <>
        struct to_value_type<std::string> {
            using type = std::string;
        };
    public:
        OptionWrapper(const std::shared_ptr<OptionBase>& option) : _option(option) {}

        explicit operator bool() const noexcept { return this->_option->use(); }

        // optionの引数を型Tとして取得する(Tがコンテナであるときはpush_backで要素を追加可能なものであるとする)
        template <class T>
        T as() const {
            using value_type = typename to_value_type<T>::type;
            OptionValue<value_type>* p = dynamic_cast<OptionValue<value_type>*>(this->_option.get());
            if (p == nullptr) {
                throw std::logic_error(std::format("option {0} から型 {1} な引数を受け取ることはできません", this->_option->full_name(), type_name<value_type>::value));
            }
            try {
                return p->template as<T>();
            }
            catch (const std::runtime_error& e) {
                throw std::runtime_error(std::format("option {0} は{1}", this->_option->full_name(), e.what()));
            }
        }
    };

    /// <summary>
    /// コマンドラインオプションのためのデータ
    /// </summary>
    class OptionMap {
        std::vector<std::shared_ptr<OptionBase>> _options;
        std::vector<std::shared_ptr<OptionBase>> _long_options;
        std::shared_ptr<OptionBase> _unnamed_options;
        /// <summary>
        /// OptionBaseで宣言されるoption
        /// </summary>
        std::vector<std::weak_ptr<OptionBase>> _ordered_options;

        /// <summary>
        /// useの実装部
        /// </summary>
        /// <param name="l">オプション名</param>
        /// <param name="options">オプションに関するリスト</param>
        /// <returns></returns>
        std::shared_ptr<OptionBase> use_impl(const std::string& l, const std::vector<std::shared_ptr<OptionBase>>& options) const {
            if (std::size_t i = l.find('='); i == l.length() - 1) {
                // lの末尾に等号「=」があれば等号で引数を受け取る対象のチェック
                std::string ll = l.substr(0, i);
                for (auto& option : options) {
                    if (option->name() == ll) {
                        auto p = dynamic_cast<OptionHasValueBase*>(option.get());
                        if (p != nullptr && (p->arg_pattern() & OptionHasValueBase::ARG_PATTERN::ASSIGN) == OptionHasValueBase::ARG_PATTERN::ASSIGN) {
                            return option;
                        }
                    }
                }
            }
            else if (std::size_t j = l.find(' '); j == l.length() - 1) {
                // lの末尾にスペース「 」があればスペースで引数を受け取る対象のチェック
                std::string ll = l.substr(0, j);
                for (auto& option : options) {
                    if (option->name() == ll) {
                        auto p = dynamic_cast<OptionHasValueBase*>(option.get());
                        if (p != nullptr && (p->arg_pattern() & OptionHasValueBase::ARG_PATTERN::SPACE) == OptionHasValueBase::ARG_PATTERN::SPACE) {
                            return option;
                        }
                    }
                }
            }
            else {
                for (auto& option : options) {
                    if (option->name() == l) {
                        return option;
                    }
                }
            }
            return nullptr;
        }

    public:
        OptionMap() {}

        /// <summary>
        /// クローンを作成する
        /// </summary>
        /// <returns>クローン</returns>
        OptionMap clone() const {
            OptionMap result;
            for (const auto& e : this->_ordered_options) {
                auto p = e.lock();
                auto option = p->clone();
                if (dynamic_cast<Option*>(option) != nullptr) {
                    result._options.emplace_back(option);
                    result._ordered_options.emplace_back(result._options.back());
                }
                else if (dynamic_cast<LongOption*>(option) != nullptr) {
                    result._long_options.emplace_back(option);
                    result._ordered_options.emplace_back(result._long_options.back());
                }
                else {
                    throw std::logic_error(std::format("option {0}は未知のoptionパターンです", p->name()));
                }
            }
            if (this->_unnamed_options) {
                result._unnamed_options.reset(this->_unnamed_options->clone());
                result._ordered_options.emplace_back(result._unnamed_options);
            }
            return result;
        }

        /// <summary>
        /// 名前なしoptionの取得
        /// </summary>
        /// <returns></returns>
        OptionWrapper unnamed_options() const {
            return OptionWrapper(this->_unnamed_options);
        }

        /// <summary>
        /// コマンドライン引数を解析する
        /// </summary>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <param name="validate">引数のチェックを行うか</param>
        /// <returns>解析後のオフセット</returns>
        int parse(int argc, const char* argv[], bool validate = true) {
            int offset = 0;
            while (offset < argc) {
                if (Option::is_option(argv[offset])) {
                    bool exist = false;
                    for (auto& ptr : this->_options) {
                        if (ptr->parse(offset, argc, argv)) {
                            // 解析に成功したときは次の解析に移る
                            exist = true;
                            break;
                        }
                    }
                    if (!exist) {
                        throw std::runtime_error(std::format("{0} に該当するoptionは存在しません", argv[offset]));
                    }
                }
                else if (LongOption::is_long_option(argv[offset])) {
                    bool exist = false;
                    for (auto& ptr : this->_long_options) {
                        if (ptr->parse(offset, argc, argv)) {
                            // 解析に成功したときは次の解析に移る
                            exist = true;
                            break;
                        }
                    }
                    if (!exist) {
                        throw std::runtime_error(std::format("{0} に該当するlong optionは存在しません", argv[offset]));
                    }
                }
                else {
                    if (OptionBase::is_dash(argv[offset])) {
                        // 次の要素が存在するならばそれを名前なしのoptionとして扱う
                        ++offset;
                        if (offset >= argc) {
                            continue;
                        }
                    }
                    if (this->_unnamed_options) {
                        if (!this->_unnamed_options->parse(offset, argc, argv)) {
                            throw std::runtime_error(std::format("これ以上の名前なしオプション {0} は設定不可です", argv[offset]));
                        }
                    }
                    else {
                        throw std::runtime_error("名前なしオプションの設定はできません");
                    }
                }
            }

            if (validate) {
                this->validate();
            }

            return offset;
        }

        /// <summary>
        /// 与えられた引数のチェック
        /// </summary>
        void validate() const {
            // 引数の正当性確認
            for (const auto& e : this->_ordered_options) {
                auto p = e.lock();
                try {
                    p->validate();
                }
                catch (const std::runtime_error& e) {
                    throw std::runtime_error(std::format("option {0} に対する{1}", p->full_name(), e.what()));
                }
            }
            if (this->_unnamed_options) {
                try {
                    this->_unnamed_options->validate();
                }
                catch (const std::runtime_error& e) {
                    throw std::runtime_error(std::format("名前なしオプションに対する引数 {0}", e.what()));
                }
            }
        }

        /// <summary>
        /// optionを利用しているかのチェック
        /// </summary>
        /// <param name="o">option名</param>
        /// <returns></returns>
        OptionWrapper ouse(const std::string& o) const {
            auto p = this->use_impl(o, this->_options);
            if (p != nullptr) {
                return OptionWrapper(p);
            }
            throw std::invalid_argument(std::format("{0} というoptionは存在しません", o));
        }

        /// <summary>
        /// long optionを利用しているかのチェック
        /// </summary>
        /// <param name="l">long option名</param>
        /// <returns></returns>
        OptionWrapper luse(const std::string& l) const {
            auto p = this->use_impl(l, this->_long_options);
            if (p != nullptr) {
                return OptionWrapper(p);
            }
            throw std::invalid_argument(std::format("{0} というlong optionは存在しません", l));
        }

        /// <summary>
        /// optionもしくはlong optionを利用しているかのチェック
        /// </summary>
        /// <param name="o">オプション名</param>
        /// <returns></returns>
        OptionWrapper use(const std::string& o) const {
            auto p1 = this->use_impl(o, this->_options);
            if (p1 != nullptr) {
                return OptionWrapper(p1);
            }
            auto p2 = this->use_impl(o, this->_long_options);
            if (p2 != nullptr) {
                return OptionWrapper(p2);
            }
            throw std::invalid_argument(std::format("{0} というoptionは存在しません", o));
        }

        /// <summary>
        /// optionの追加
        /// </summary>
        /// <param name="option">追加するoption</param>
        void add_option(Option* option) {
            this->_options.emplace_back(option);
            this->_ordered_options.emplace_back(this->_options.back());
        }

        /// <summary>
        /// long optionの追加
        /// </summary>
        /// <param name="option">追加するoption</param>
        void add_long_option(LongOption* option) {
            this->_long_options.emplace_back(option);
            this->_ordered_options.emplace_back(this->_long_options.back());
        }

        /// <summary>
        /// 名前なしのオプションの追加
        /// </summary>
        /// <param name="option">追加するoption</param>
        template <class T>
        void add_unnamed_option(UnnamedOption<T>* option) {
            if (this->_unnamed_options) {
                throw std::invalid_argument("複数の名前なしオプションは定義できません");
            }
            this->_unnamed_options.reset(option);
            this->_ordered_options.emplace_back(this->_unnamed_options);
        }

        /// <summary>
        /// optionの説明を取得する
        /// </summary>
        /// <param name="optionCols">option部の列数</param>
        /// <param name="lengthBetweenOptionAndDescription">option名と説明の間の隙間</param>
        /// <returns></returns>
        std::string description(std::size_t optionCols, std::size_t lengthBetweenOptionAndDescription) const {
            std::ostringstream oss;
            for (const auto& e : this->_ordered_options) {
                auto p = e.lock();
                auto desc = p->description();
                auto name_desc = p->name_description();
                oss << "  " << name_desc;
                // optionが長すぎるときは適当に補間する
                if (optionCols < name_desc.length() + lengthBetweenOptionAndDescription) {
                    oss << std::string(lengthBetweenOptionAndDescription, ' ');
                }
                else {
                    oss << std::string(optionCols - name_desc.length(), ' ');
                }
                oss << desc << std::endl;
            }
            if (this->_ordered_options.empty()) oss << "  None" << std::endl;
            return oss.str();
        }

        /// <summary>
        /// コマンドライン解析前の状態へ初期化
        /// </summary>
        void init() {
            for (const auto& e : this->_ordered_options) {
                e.lock()->init();
            }
        }
    };

    /// <summary>
    /// オプションの追加の記述のためのET
    /// </summary>
    class AddOptions {
        OptionMap& _option_map;

        /// <summary>
        /// optionの構築のためのクラス
        /// </summary>
        class OptionBuilder {
            AddOptions& _ao;
        public:
            OptionBuilder() = delete;
            OptionBuilder(AddOptions& ao) : _ao(ao) {}

            /// <summary>
            /// optionの生成
            /// </summary>
            /// <param name="name">option名</param>
            /// <param name="desc">optionの説明</param>
            /// <returns></returns>
            AddOptions& operator()(const std::string& name, const std::string& desc) {
                this->_ao._option_map.add_option(new Option(name, desc));
                return this->_ao;
            }

            /// <summary>
            /// 引数付きのoptionの生成
            /// </summary>
            /// <typeparam name="T">引数の型</typeparam>
            /// <param name="name">option名</param>
            /// <param name="value">引数の情報</param>
            /// <param name="desc">optionの説明</param>
            /// <returns></returns>
            template <class T>
            AddOptions& operator()(const std::string& name, const Value<T>& value, const std::string& desc) {
                this->_ao._option_map.add_option(new OptionHasValue<T>(value, name, desc));
                return this->_ao;
            }
        };

        /// <summary>
        /// long optionの構築のためのクラス
        /// </summary>
        class LongOptionBuilder {
            AddOptions& _ao;
        public:
            LongOptionBuilder() = delete;
            LongOptionBuilder(AddOptions& ao) : _ao(ao) {}

            /// <summary>
            /// long optionの生成
            /// </summary>
            /// <typeparam name="T">引数の型</typeparam>
            /// <param name="name">long option名</param>
            /// <param name="desc">long optionの説明</param>
            /// <returns></returns>
            AddOptions& operator()(const std::string& name, const std::string& desc) {
                this->_ao._option_map.add_long_option(new LongOption(name, desc));
                return this->_ao;
            }

            /// <summary>
            /// 引数付きlong optionの生成
            /// </summary>
            /// <typeparam name="T">引数の型</typeparam>
            /// <param name="name">long option名</param>
            /// <param name="value">引数の情報</param>
            /// <param name="desc">long optionの説明</param>
            /// <returns></returns>
            template <class T>
            AddOptions& operator()(const std::string& name, const Value<T>& value, const std::string& desc) {
                LongOptionHasValue<T>* temp = nullptr;
                std::size_t i = name.find('=');
                std::size_t j = name.find(' ');
                if (i == name.length() - 1) {
                    temp = new LongOptionHasValue<T>(value, name.substr(0, i), desc, OptionHasValueBase::ARG_PATTERN::ASSIGN);
                }
                else if (j == name.length() - 1) {
                    temp = new LongOptionHasValue<T>(value, name.substr(0, j), desc, OptionHasValueBase::ARG_PATTERN::SPACE);
                }
                else {
                    temp = new LongOptionHasValue<T>(value, name, desc, OptionHasValueBase::ARG_PATTERN::ASSIGN | OptionHasValueBase::ARG_PATTERN::SPACE);
                }

                this->_ao._option_map.add_long_option(temp);
                return this->_ao;
            }
        };
        /// <summary>
        /// 名前なしオプションの構築のためのクラス
        /// </summary>
        class UnnamedOptionBuilder {
            AddOptions& _ao;
            /// <summary>
            /// trueなら後続の解析を中断する
            /// </summary>
            bool _pause = false;

            /// <summary>
            /// UnnamedOptionBuilderの設定値を初期化する
            /// </summary>
            void init() {
                this->_pause = false;
            }

        public:
            UnnamedOptionBuilder() = delete;
            UnnamedOptionBuilder(AddOptions& ao) : _ao(ao) {}

            /// <summary>
            /// 後続の解析を中断することの宣言
            /// </summary>
            /// <returns></returns>
            UnnamedOptionBuilder& pause() {
                this->_pause = true;
                return *this;
            }

            /// <summary>
            /// 名前なしオプションの生成
            /// </summary>
            /// <typeparam name="T">引数の型</typeparam>
            /// <param name="value">引数の情報</param>
            /// <param name="desc">名前なしオプションの説明</param>
            /// <returns></returns>
            template <class T>
            AddOptions& operator()(const Value<T>& value, const std::string& desc) {
                UnnamedOption<T>* temp = new UnnamedOption<T>(value, desc);
                temp->_pause = this->_pause;
                this->_ao._option_map.add_unnamed_option(temp);
                this->init();
                return this->_ao;
            }
        };
    public:
        AddOptions(OptionMap& option_map) : _option_map(option_map), o(*this), l(*this), u(*this) {}

        /// <summary>
        /// optionの構築のためのクラス
        /// </summary>
        OptionBuilder o;
        /// <summary>
        /// long optionの構築のためのオブジェクト
        /// </summary>
        LongOptionBuilder l;
        /// <summary>
        /// 名前なしオプションの構築のためのオブジェクト
        /// </summary>
        UnnamedOptionBuilder u;
    };

    /// <summary>
    /// コマンドライン引数の解析を行うために宣言をするクラス
    /// </summary>
    class CommandLineOption {
        OptionMap _map;
    public:
        CommandLineOption() {}

        /// <summary>
        /// オプションに関するmapの取得
        /// </summary>
        /// <returns></returns>
        const OptionMap& map() const { return this->_map; }

        /// <summary>
        /// オプションの追加のためのAddOptionsの生成
        /// </summary>
        /// <returns></returns>
        AddOptions add_options() { return AddOptions(this->_map); }

        /// <summary>
        /// コマンドライン引数のオプションを解析する
        /// </summary>
        /// <param name="argc">コマンドライン引数の数</param>
        /// <param name="argv">コマンドライン引数を示す配列</param>
        /// <param name="validate">引数のチェックを行うか</param>
        /// <returns>解析後のオフセット</returns>
        int parse(int argc, const char* argv[], bool validate = true) {
            return this->_map.parse(argc, argv, validate);
        }

        /// <summary>
        /// コマンドラインオプションの説明の取得
        /// </summary>
        /// <returns></returns>
        std::string description() const {
            return this->_map.description(this->optionCols, this->lengthBetweenOptionAndDescription);
        }

        /// <summary>
        /// コマンドラインオプションの説明におけるoption部の列数
        /// </summary>
        std::size_t optionCols = 25;
        /// <summary>
        /// コマンドラインオプションの説明におけるoption名と説明の間の隙間
        /// </summary>
        std::size_t lengthBetweenOptionAndDescription = 2;
    };
}
