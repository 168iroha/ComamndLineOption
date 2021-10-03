/*
 * コマンドライン引数の簡単な解析をする
 * この実装ではoptionと引数の連結を許容していない
 * ただし、long optionの等号での連結は許容している
 * long optionの省略も許容していない
 */
#ifndef COMMAND_LINE_OPTION_HPP
#define COMMAND_LINE_OPTION_HPP

#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>

namespace option {

    /*
    * strを型Tのフォーマットに変換して返す
    * 変換できないときは文字列をそのまま返す
    * @param str : 変換対象の文字列
    */
    template <class T>
    inline constexpr auto to_numeric(const std::string& str) {
        if constexpr (std::is_same_v<T, char>) {
            return str[0];
        }
        else if constexpr (std::is_same_v<T, int>) {
            return std::stoi(str);
        }
        else if constexpr (std::is_same_v<T, long>) {
            return std::stol(str);
        }
        else if constexpr (std::is_same_v<T, long long>) {
            return std::stoll(str);
        }
        else if constexpr (std::is_same_v<T, unsigned long long>) {
            return std::stoull(str);
        }
        else if constexpr (std::is_same_v<T, float>) {
            return std::stof(str);
        }
        else if constexpr (std::is_same_v<T, double>) {
            return std::stod(str);
        }
        else if constexpr (std::is_same_v<T, long double>) {
            return std::stold(str);
        }
        else return str;
    }
    /*
    * 実装定義の型名name(typeid(T).name()で取得)から型名を取得
    * 型を取得できないときは文字列Unknownを返す
    * @param name : 型を取得する対象の文字列
    */
    template <class T>
    inline constexpr  std::string type_name() {
        const std::vector<std::pair<std::string, std::string>> table = {
            { typeid(std::string).name(), "std::string" },
            { typeid(int).name(), "int" },
            { typeid(long).name(), "long" },
            { typeid(long long).name(), "long long" },
            { typeid(unsigned long long).name(), "unsigned long long" },
            { typeid(float).name(), "float" },
            { typeid(double).name(), "double" },
            { typeid(long double).name(), "long double" },
        };
        const std::string name = typeid(T).name();

        if (auto itr = std::find_if(table.begin(), table.end(),
            [&](const std::pair<std::string, std::string>& x) { return x.first == name; }); itr != table.end()) {
            return itr->second;
        }
        else return "Unknwon";
    }

    // optionのパターン
    enum struct OPTION_PATTERN {
        OPTION,             // option(-tokenの形式)
        LONG_OPTION,        // long option(--tokenの形式)
    };

    // optionの引数のパターン
    enum struct OPTION_ARG_PATTERN {
        NONE = 0,                   // 利用不可
        NEXT_ARG = 1,               // --aaa bのような次の引数をoptionの引数にする
        EQUAL_SIGN = 2,             // --aaa=bのような等号による指定(long option限定)
        ALL_AVAILABLE = 3,          // 全ての利用可能な形式が可能
    };
    // optionの引数パターンの取得
    extern bool check_pattern(OPTION_ARG_PATTERN ref, OPTION_ARG_PATTERN pattern);

    // optionの一般の記述を取得
    extern std::string description(const std::string& option, const std::string& arg_name, OPTION_PATTERN option_pattern, OPTION_ARG_PATTERN arg_pattern, int limit, const std::string& default_value);
    extern std::string description(const std::string& option, const std::string& arg_name, OPTION_PATTERN option_pattern, OPTION_ARG_PATTERN arg_pattern, int limit);

    // option
    class Option {
        std::string name_m;             // オプション名
        std::string descript_m;         // オプションの説明
        bool use_m;                     // オプションを使用しているか
        OPTION_PATTERN pattern_m;
    public:
        Option(const std::string& name, const std::string& desc, OPTION_PATTERN pattern);
        virtual ~Option() {}

        virtual Option* clone() const { return new Option(*this); }

        // 引数を追加する
        virtual void add_value_s(const std::string& value_s);
        // 引数が利用可能か
        virtual OPTION_ARG_PATTERN useable_argument() const { return OPTION_ARG_PATTERN::NONE; }

        const std::string& name() const { return this->name_m; }
        const std::string& descript() const { return this->descript_m; }
        OPTION_PATTERN option_pattern() const { return this->pattern_m; }
        bool use() const { return this->use_m; }
        void use(bool f) { this->use_m = f; }

        // -や--を含めたoptionの名前を取得
        std::string full_option_name() const;

        // 説明の取得
        virtual std::pair<std::string, std::string> description() const;
    };

    // 引数のオプション
    template <class T>
    class Value {
        // デフォルト引数
        std::vector<T> default_value_m;
        // 引数の制約条件
        std::function<bool(T)> constraint_m;
        // 引数の数の制限
        int limit_m = 1;
        // 引数の表示名(helpで<name_m...[1-limit_m]>のように表示される)
        std::string name_m = "arg";
    public:
        Value() {}
        Value(const T& x) : default_value_m(1, x) {}
        Value(std::initializer_list<T> x) : default_value_m(x) {}

        template <class F>
        Value& constraint(F f);
        Value& limit(int l) {
            if (l == 0) throw std::logic_error("保持する引数の数は0に設定することはできません");
            if (l < this->default_value_m.size()) throw std::logic_error("デフォルト引数の数が引数の数の制限を超過しています");
            this->limit_m = l;
            return *this;
        }
        // 引数をいくらでも取ることが可能にする
        Value& unlimited() {
            this->limit_m = -1;
            return *this;
        }
        Value& name(const std::string& n) {
            this->name_m = n;
            return *this;
        }

        bool use_default_value() const { return !this->default_value_m.empty(); }
        const std::vector<T>& default_value() const { return this->default_value_m; }
        const std::function<bool(T)>& constraint() const { return this->constraint_m; }
        int limit() const { return this->limit_m; }
        const std::string& name() const { return this->name_m; }
    };

    // 引数をもつoption
    template <class T>
    class OptionHasValue : public Option {
        friend class Value<T>;
        
        Value<T> value_info_m;          // 引数に関する設定項目
        std::vector<T> value_m;         // 受け取った引数本体
        OPTION_ARG_PATTERN arg_pattern_m;

        // デフォルト引数をカンマ区切りの文字列に変換
        static std::string to_string(const std::vector<T>& x) {
            std::stringstream stream;
            stream << x[0];
            for (std::size_t i = 1; i < x.size(); ++i)
                stream << "," << x[i];
            return stream.str();
        }
        static std::string to_string(const T& x) {
            std::stringstream stream;
            stream << x;
            return stream.str();
        }
    public:
        OptionHasValue(const Value<T>& value_info, const std::string& name, const std::string& desc, OPTION_PATTERN pattern, OPTION_ARG_PATTERN arg_pattern = OPTION_ARG_PATTERN::ALL_AVAILABLE)
            : Option(name, desc, pattern), value_info_m(value_info), arg_pattern_m(arg_pattern) {
            if (arg_pattern == OPTION_ARG_PATTERN::NONE) {
                throw std::logic_error("引数をもつoptionに対して、引数をもたないようにする指定はできません");
            }
        }
        virtual ~OptionHasValue() {}

        virtual Option* clone() const {
            return new OptionHasValue(*this);
        }

        // 引数の追加
        void add_value(const T& value) {
            // 制約式が存在すればチェックする
            if (this->value_info_m.constraint()) {
                if (!this->value_info_m.constraint()(value)) {
                    throw std::runtime_error("option " + this->full_option_name() + " に対する引数 " + OptionHasValue::to_string(value) + " は制約条件を満たしていません");
                }
            }
            // 初めての引数の追加のときはクリアする
            if (!this->use()) this->value_m.clear();
            // 単純に引数の追加
            if ((this->value_info_m.limit() < 0) || (this->value_m.size() < static_cast<std::size_t>(this->value_info_m.limit()))) this->value_m.push_back(value);
            // 一番最後の要素に対して上書きする
            else this->value_m[this->value_info_m.limit() - 1] = value;
            this->use(true);
        }
        void add_value(const std::vector<T>& values) {
            for (const auto& value : values) this->add_value(value);
        }
        virtual void add_value_s(const std::string& value_s) {
            T value = T();
            // 受け取った文字列を型Tに変換する
            try {
                value = to_numeric<T>(value_s);
            } catch (const std::logic_error& e) {
                throw std::runtime_error("option " + this->full_option_name() + " に対する引数 " + value_s + " は型 " + type_name<T>() + " に変換することはできません");
            }
            this->add_value(value);
        }
        virtual OPTION_ARG_PATTERN useable_argument() const { return this->arg_pattern_m; }

        template <class U>
        U as() const {
            if (this->value_m.size() == 0) {
                throw std::runtime_error("option " + this->full_option_name() + " は引数をもっていません");
            }
            if constexpr (std::is_same_v<T, U>) {
                return this->value_m[0];
            }
            else {
                U result;
                for (const T& e : this->value_m) result.push_back(e);
                return result;
            }
        };

        // 説明の取得
        virtual std::pair<std::string, std::string> description() const {
            std::string desc;
            if (this->value_info_m.use_default_value()) {
                desc = ::option::description(this->name(), this->value_info_m.name(), this->option_pattern(), this->arg_pattern_m, this->value_info_m.limit()
                    , OptionHasValue::to_string(this->value_info_m.default_value()));
            }
            else {
                desc = ::option::description(this->name(), this->value_info_m.name(), this->option_pattern(), this->arg_pattern_m, this->value_info_m.limit());
            }
            return { desc, this->descript() };
        }
    };

    template <class T>
    template <class F>
    inline Value<T>& Value<T>::constraint(F f) {
        // デフォルト引数が制約条件を満たしているかのチェック
        for (const auto& value : this->default_value_m) {
            if (!f(value)) {
                throw std::logic_error("デフォルト引数 " + OptionHasValue<T>::to_string(value) + " は制約条件を満たしていません");
            }
        }
        this->constraint_m = f;
        return *this;
    }

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

    class OptionWrapper {
        const std::shared_ptr<Option>& option_m;
    public:
        OptionWrapper(const std::shared_ptr<Option>& option) : option_m(option) {}

        explicit operator bool() const noexcept { return this->option_m->use(); }

        // optionの引数を型Tとして取得する(Tがコンテナであるときはpush_backで要素を追加可能なものであるとする)
        template <class T>
        T as() const {
            if (check_pattern(this->option_m->useable_argument(), OPTION_ARG_PATTERN::NONE)) {
                throw std::logic_error("option " + this->option_m->full_option_name() + " から引数を受け取ることはできません");
            }
            using value_type = typename to_value_type<T>::type;
            OptionHasValue<value_type>* p = dynamic_cast<OptionHasValue<value_type>*>(this->option_m.get());
            if (p == nullptr) {
                throw std::logic_error("option " + this->option_m->full_option_name() + " から型な" + type_name<value_type>() + " な引数を受け取ることはできません");
            }
            return p->template as<T>();
        }
    };

    // コマンドラインオプションのためのデータ
    class OptionMap {
        std::vector<std::shared_ptr<Option>> options_m;
        std::vector<std::shared_ptr<Option>> long_options_m;

        // optionの追加順序を記憶するための参照
        std::vector<std::weak_ptr<Option>> order_options_m;

        std::vector<std::string> none_options_m;     // optionではなかった引数
    public:
        OptionMap() {}

        // 複製の構築
        OptionMap clone() const;

        std::vector<std::shared_ptr<Option>>& options() { return this->options_m; }
        const std::vector<std::shared_ptr<Option>>& options() const { return this->options_m; }
        std::vector<std::shared_ptr<Option>>& long_options() { return this->long_options_m; }
        const std::vector<std::shared_ptr<Option>>& long_options() const { return this->long_options_m; }

        std::vector<std::weak_ptr<Option>>& order_options() { return this->order_options_m; }
        const std::vector<std::weak_ptr<Option>>& order_options() const { return this->order_options_m; }

        std::vector<std::string>& none_options() { return this->none_options_m; }
        const std::vector<std::string>& none_options() const { return this->none_options_m; }

        /*
        * optionを使用しているかのチェック
        * @param o : option名
        */
        OptionWrapper ouse(const std::string& o) const;
        /*
        * long optionを使用しているかのチェック
        * @param l : long option名
        */
        OptionWrapper luse(const std::string& l) const;
        /*
        * optionもしくはlong optionを使用しているかのチェック
        * @param l : option名
        */
        OptionWrapper use(const std::string& o) const;
    };

    // オプションの追加のためのET
    class AddOptions {
        OptionMap& option_map_m;
    public:
        AddOptions(OptionMap& option_map) : option_map_m(option_map) {}

        // long optionの生成
        AddOptions& l(const std::string& name, const std::string& desc) {
            this->option_map_m.long_options().emplace_back(new Option(name, desc, OPTION_PATTERN::LONG_OPTION));
            this->option_map_m.order_options().emplace_back(this->option_map_m.long_options().back());
            return *this;
        }
        /*
        * nameの末尾に等号'='があるとき、等号'='でのみ引数を受け取る
        * これは引数を受け取らないlong optionと共存することができる
        * 同様に空白スペース' 'があるときは、次のコマンドライン引数でのみ引数を受け取る
        */
        template <class T>
        AddOptions& l(const std::string& name, const Value<T>& value, const std::string& desc) {
            OptionHasValue<T>* temp = nullptr;
            if (std::size_t i = name.find('='); i == name.length() - 1) {
                temp = new OptionHasValue<T>(value, name.substr(0, i), desc, OPTION_PATTERN::LONG_OPTION, OPTION_ARG_PATTERN::EQUAL_SIGN);
            }
            else if (std::size_t i = name.find(' '); i == name.length() - 1) {
                temp = new OptionHasValue<T>(value, name.substr(0, i), desc, OPTION_PATTERN::LONG_OPTION, OPTION_ARG_PATTERN::NEXT_ARG);
            }
            else {
                temp = new OptionHasValue<T>(value, name, desc, OPTION_PATTERN::LONG_OPTION);
            }
            if (value.use_default_value()) temp->add_value(value.default_value());
            this->option_map_m.long_options().emplace_back(temp);
            this->option_map_m.order_options().emplace_back(this->option_map_m.long_options().back());
            return *this;
        }
        // optionの生成
        AddOptions& o(const std::string& name, const std::string& desc) {
            this->option_map_m.options().emplace_back(new Option(name, desc, OPTION_PATTERN::OPTION));
            this->option_map_m.order_options().emplace_back(this->option_map_m.options().back());
            return *this;
        }
        template <class T>
        AddOptions& o(const std::string& name, const Value<T>& value, const std::string& desc) {
            OptionHasValue<T>* temp = new OptionHasValue<T>(value, name, desc, OPTION_PATTERN::OPTION);
            if (value.use_default_value()) temp->add_value(value.default_value());
            this->option_map_m.options().emplace_back(temp);
            this->option_map_m.order_options().emplace_back(this->option_map_m.options().back());
            return *this;
        }
    };

    class CommandLineOption {
        OptionMap map_m;
    public:
        CommandLineOption() {}

        const OptionMap& map() const { return this->map_m; }

        // オプションの追加
        AddOptions add_options() { return AddOptions(this->map_m); }

        // コマンドライン引数の解析
        OptionMap parse(int argc, const char *argv[]);

        // optionの説明の取得
        std::string description() const;
    };
}

#endif