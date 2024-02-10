/*
 * コマンドライン引数の簡単な解析をする
 * この実装ではoptionと引数の連結を許容していない
 * ただし、long optionの等号での連結は許容している
 * long optionの省略も許容していない
 */
#include "CommandLineOption.hpp"
#include <iostream>

namespace option {

    // strがoptionであるか
    static bool is_option(const char* str) {
        if (str == nullptr) return false;
        return *str == '-' && *(str + 1) != '-' && *(str + 1) != '\0';
    }
    // strがlong optionであるか
    static bool is_long_option(const char* str) {
        if (str == nullptr) return false;
        return *str == '-' && *(str + 1) == '-' && *(str + 2) != '-' && *(str + 2) != '\0';
    }

    // inputをdelimiterで分割する
    static std::vector<std::string> split(const std::string& input, char delimiter) {
        std::istringstream stream(input);
        std::string field;
        std::vector<std::string> result;
        while (std::getline(stream, field, delimiter)) result.push_back(field);
        return result;
    }

    // optionの引数パターンの取得
    bool check_pattern(OPTION_ARG_PATTERN ref, OPTION_ARG_PATTERN pattern) {
        if (pattern == OPTION_ARG_PATTERN::NONE) return ref == pattern;
        return (int(ref) & int(pattern)) == int(pattern);
    }

    // optionの形式の取得
    static std::string description_name(const std::string& option, OPTION_PATTERN option_pattern) {
        std::string name;
        switch (option_pattern) {
            case OPTION_PATTERN::OPTION:
                name = "-" + option;
                break;
            case OPTION_PATTERN::LONG_OPTION:
                name = "--" + option;
                break;
            default:
                throw std::logic_error("未知のoption patternを使用しています");
        }
        return name;
    }
    // 引数の形式の取得
    static std::string description_arg(const std::string& arg_name, int limit, const std::string& default_value) {
        // 引数の形式の取得
        std::string arg = "<";
        arg += arg_name;
        if (limit < 0) {
            arg += "...";
        }
        else if (limit > 1) {
            arg += "...[1-" + std::to_string(limit) + "]";
        }
        arg += ">";
        arg += "(=" + default_value + ")";
        return arg;
    }
    // 引数の形式の取得
    static std::string description_arg(const std::string& arg_name, int limit) {
        // 引数の形式の取得
        std::string arg = "<";
        arg += arg_name;
        if (limit < 0) {
            arg += "...";
        }
        else if (limit > 1) {
            arg += "...[1-" + std::to_string(limit) + "]";
        }
        arg += ">";
        return arg;
    }
    // 引数の受け取り方の記述
    static std::string description_op(OPTION_PATTERN option_pattern, OPTION_ARG_PATTERN arg_pattern) {
        std::string op;
        switch (arg_pattern) {
            case OPTION_ARG_PATTERN::NEXT_ARG:
                op = " ";
                break;
            case OPTION_ARG_PATTERN::EQUAL_SIGN:
                op = "=";
                break;
            case OPTION_ARG_PATTERN::ALL_AVAILABLE:
                if (option_pattern == OPTION_PATTERN::OPTION) op = " ";
                else op = "[ |=]";
                break;
            default:
                throw std::logic_error("未知の引数の受け取り方を指定しています");
        }
        return op;
    }
    // optionの一般の記述を取得
    std::string description(const std::string& option, const std::string& arg_name, OPTION_PATTERN option_pattern, OPTION_ARG_PATTERN arg_pattern, int limit, const std::string& default_value) {
        return description_name(option, option_pattern) + description_op(option_pattern, arg_pattern) + description_arg(arg_name, limit, default_value);
    }
    std::string description(const std::string& option, const std::string& arg_name, OPTION_PATTERN option_pattern, OPTION_ARG_PATTERN arg_pattern, int limit) {
        return description_name(option, option_pattern) + description_op(option_pattern, arg_pattern) + description_arg(arg_name, limit);
    }

    Option::Option(const std::string& name, const std::string& desc, OPTION_PATTERN pattern) : name_m(name), descript_m(desc), use_m(false), pattern_m(pattern) {
        if (name.length() == 0) {
            throw std::logic_error("空のoption名は定義することはできません");
        }
        if (name[0] == '-') {
            throw std::logic_error("option名の1文字目は'-'にすることはできません");
        }
        if (name.find('=') != std::string::npos) {
            throw std::logic_error("optionに等号を含めることはできません");
        }
        if (name.find(' ') != std::string::npos) {
            throw std::logic_error("optionに空白スペースを含めることはできません");
        }
    }

    // 引数を追加する
    void Option::add_value_s(const std::string& value_s) {
        throw std::runtime_error("option " + this->full_option_name() + " で引数を受け取ることはできません");
    }

    // -や--を含めたoptionの名前を取得
    std::string Option::full_option_name() const {
        switch (this->pattern_m) {
            case OPTION_PATTERN::OPTION:
                return "-" + this->name_m;
            case OPTION_PATTERN::LONG_OPTION:
                return "--" + this->name_m;
            default:
                throw std::logic_error("未知のoption patternを使用しています");
        }
    }
    // 説明の取得
    std::pair<std::string, std::string> Option::description() const {
        switch (this->option_pattern()) {
            case OPTION_PATTERN::OPTION:
                return { "-" + this->name(), this->descript() };
            case OPTION_PATTERN::LONG_OPTION:
                return { "--" + this->name(), this->descript() };
            default:
                throw std::logic_error("未知のoption patternを使用しています");
        }
    }

    // 複製の構築
    OptionMap OptionMap::clone() const {
        OptionMap result;
        for (const auto& e : this->order_options_m) {
            auto p = e.lock();
            switch (p->option_pattern()) {
                case OPTION_PATTERN::OPTION:
                    result.options_m.emplace_back(p->clone());
                    result.order_options_m.emplace_back(result.options_m.back());
                    break;
                case OPTION_PATTERN::LONG_OPTION:
                    result.long_options_m.emplace_back(p->clone());
                    result.order_options_m.emplace_back(result.long_options_m.back());
                    break;
                default:
                    throw std::logic_error("option " + p->name() + "は未知のoptionパターンです");
            }
        }
        for (const auto& e : this->none_options_m) result.none_options_m.push_back(e);
        return result;
    }
    /*
    * optionを使用しているかのチェック
    * @param o : option名
    */
    OptionWrapper OptionMap::ouse(const std::string& o) const {
        for (auto& option : this->options_m) {
            if (option->name() == o) {
                return OptionWrapper(option);
            }
        }
        throw std::logic_error("-" + o + " というoptionは存在しません");
    }
    /*
    * long optionを使用しているかのチェック
    * @param l : long option名
    */
    OptionWrapper OptionMap::luse(const std::string& l) const {
        std::size_t i = l.find('=');
        // lの末尾に等号'='があれば等号で引数を受け取る対象のチェック
        if (i == l.length() - 1) {
            std::string ll = l.substr(0, i);
            for (auto& option : this->long_options_m) {
                if ((option->name() == ll) && (check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::EQUAL_SIGN))) {
                    return OptionWrapper(option);
                }
            }
        }
        else {
            for (auto& option : this->long_options_m) {
                if (option->name() == l) {
                    return OptionWrapper(option);
                }
            }
        }
        throw std::logic_error("--" + l + " というlong optionは存在しません");
    }
    /*
    * optionもしくはlong optionを使用しているかのチェック
    * @param l : option名
    */
    OptionWrapper OptionMap::use(const std::string& o) const {
        std::size_t i = o.find('=');
        std::size_t j = o.find(' ');
        // lの末尾に等号'='があれば等号で引数を受け取る対象のチェック
        if (i == o.length() - 1) {
            std::string oo = o.substr(0, i);
            for (auto& option : this->long_options_m) {
                if ((option->name() == oo) && (check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::EQUAL_SIGN))) {
                    return OptionWrapper(option);
                }
            }
        }
        // lの末尾に空白スペース' 'があれば次の引数optionので引数を受け取る対象のチェック
        else if (j == o.length() - 1) {
            std::string oo = o.substr(0, j);
            for (auto& option : this->long_options_m) {
                if ((option->name() == oo) && (check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::NEXT_ARG))) {
                    return OptionWrapper(option);
                }
            }
        }
        else {
            for (auto& option : this->options_m) {
                if (option->name() == o) {
                    return OptionWrapper(option);
                }
            }
            for (auto& option : this->long_options_m) {
                if (option->name() == o) {
                    return OptionWrapper(option);
                }
            }
        }
        throw std::logic_error(o + " というoptionおよびlong optionは存在しません");
    }

    // コマンドライン引数の解析
    OptionMap CommandLineOption::parse(int argc, const char *argv[]) {
        OptionMap result = this->map_m.clone();
        int p = 1;
        while (p < argc) {
            if (is_option(argv[p])) {
                bool exist = false;
                for (auto& option : result.options()) {
                    if (option->full_option_name() == argv[p]) {
                        // 次の要素を引数として利用するとき
                        if (check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::NEXT_ARG)) {
                            if (p + 1 >= argc || is_option(argv[p + 1]) || is_long_option(argv[p + 1])) {
                                throw std::runtime_error("option " + option->full_option_name() + " には引数を指定する必要があります");
                            }
                            option->add_value_s(argv[p + 1]);
                            p += 2;
                        }
                        else {
                            option->use(true);
                            ++p;
                        }
                        exist = true;
                        break;
                    }
                }
                if (!exist) {
                    throw std::runtime_error(argv[p] + std::string(" に該当するoptionは存在しません"));
                }
            }
            else if (is_long_option(argv[p])) {
                bool exist = false;
                std::string value_s = std::string(argv[p]);
                std::size_t i = value_s.find('=');
                
                for (auto& option : result.long_options()) {
                    // long optionでは等号の記述を許容するため等号で区切る
                    if (option->full_option_name() == value_s.substr(0, i)) {
                        // argv[p]内で等号により引数が指定されているとき
                        if ((i != std::string::npos) && check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::EQUAL_SIGN)) {
                            // 等号が最後の位置であるときは例外を投げる
                            if (i == value_s.length() - 1) {
                                throw std::runtime_error("=の後には引数を明示的に指定する必要があります");
                            }
                            // カンマ区切りで複数の引数を得る
                            std::vector<std::string> values_s = split(value_s.substr(i + 1), ',');
                            for (const auto& e : values_s) {
                                option->add_value_s(e);
                            }
                            ++p;
                            exist = true;
                            break;
                        }
                        // 次の要素を引数として利用するとき
                        else if (i == std::string::npos) {
                            if (check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::NEXT_ARG)) {
                                if (p + 1 >= argc || is_option(argv[p + 1]) || is_long_option(argv[p + 1])) {
                                    throw std::runtime_error("option " + option->full_option_name() + " には引数を指定する必要があります");
                                }
                                option->add_value_s(argv[p + 1]);
                                p += 2;
                            }
                            // 引数を利用しないとき
                            else if (check_pattern(option->useable_argument(), OPTION_ARG_PATTERN::NONE)) {
                                option->use(true);
                                ++p;
                            }
                            exist = true;
                            break;
                        }
                    }
                }
                if (!exist) {
                    throw std::runtime_error(argv[p] + std::string(" に該当するoptionは存在しません"));
                }
            }
            else {
                result.none_options().push_back(argv[p]);
                ++p;
            }
        }
        return result;
    }

    // optionの説明の取得
    std::string CommandLineOption::description() const {
        std::ostringstream oss;
        for (const auto& e : this->map_m.order_options()) {
            auto p = e.lock();
            auto desc = p->description();
            oss << "  " << desc.first;
            // 基本的にoptionは25文字で記述する
            // 超過するときは2つ半角スペースを挟む
            if (25 - 2 < desc.first.length()) {
                oss << "  ";
            }
            else {
                oss << std::string(25 - desc.first.length(), ' ');
            }
            oss << desc.second << std::endl;
        }
        if (this->map_m.order_options().empty()) oss << "  None" << std::endl;
        return oss.str();
    }
}
