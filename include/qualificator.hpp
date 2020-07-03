// ============================== QUALIFICATOR ============================== //
// Project:         Qualificator
// Name:            qualificator.hpp
// Description:     A utility to inspect C++ qualifiers in C++
// Creator:         Vincent Reverdy
// Contributor(s):  Vincent Reverdy [2020-]
// License:         BSD 3-Clause License
// ========================================================================== //
#ifndef _QUALIFICATOR_HPP_INCLUDED
#define _QUALIFICATOR_HPP_INCLUDED
// ========================================================================== //



// ============================== PREAMBLE ================================== //
// C++ standard library
#include <string>
#include <cstdlib>
#include <fstream>
#include <utility>
#include <iostream>
#include <type_traits>
// Project sources
// Third-party libraries
// Miscellaneous
namespace qualificator {
// ========================================================================== //



// ================================= HELPERS ================================ //
// A type placeholder
struct type {};

// To get the full name
struct full {};

// To get the short name
struct terse {};

// To get the east-const version
struct east {};

// To get the west-const version
struct west {};
// ========================================================================== //



// ================================== PACK ================================== //
// An index type to index packs of types
template <std::size_t I>
struct pack_index_type: std::integral_constant<std::size_t, I> {};

// An index value to index pack of types
template <std::size_t I>
inline constexpr pack_index_type<I> pack_index;

// A wrapper to wrap an indexed element of a pack of types
template <std::size_t I, class T>
struct pack_element {
    using type = T;
    using index_type = pack_index_type<I>;
    static constexpr index_type index = pack_index<I>;
    constexpr pack_element operator()(index_type) const noexcept {
        return *this;
    }
};

// The base of a pack of types: declaration
template <class Indices, class... Args>
struct pack_base;

// The base of a pack of types: definition
template <std::size_t... I, class... Args>
struct pack_base<std::index_sequence<I...>, Args...>: pack_element<I, Args>... {
    using index_sequence = std::index_sequence<I...>;
    using size_type = std::size_t;
    using pack_element<I, Args>::operator()...;
    static constexpr size_type size() {
        return sizeof...(Args);
    }
    template <class F>
    static constexpr void for_each(F&& f) {
        ((void)std::forward<F>(f)(pack_element<I, Args>{}), ...);
    }
};

// A pack of types
template <class... Args>
struct pack: pack_base<std::index_sequence_for<Args...>, Args...> {};

// Returns the element type of a pack of types
template <class Pack, std::size_t I>
using pack_element_t = decltype(std::declval<Pack>()(pack_index<I>));

// Returns size of a pack of types
template <class Pack>
inline constexpr typename Pack::size_type pack_size_v = Pack::size();

// Returns a pack of qualifiers
template <class T>
using make_qualified_pack = pack<
    T,
    std::add_const_t<T>,
    std::add_volatile_t<T>,
    std::add_cv_t<T>,
    std::add_lvalue_reference_t<T>,
    std::add_lvalue_reference_t<std::add_const_t<T>>,
    std::add_lvalue_reference_t<std::add_volatile_t<T>>,
    std::add_lvalue_reference_t<std::add_cv_t<T>>,
    std::add_rvalue_reference_t<T>,
    std::add_rvalue_reference_t<std::add_const_t<T>>,
    std::add_rvalue_reference_t<std::add_volatile_t<T>>,
    std::add_rvalue_reference_t<std::add_cv_t<T>>
>;

// Pack of packs: declaration
template <class...>
struct packs;

// Pack of packs: empty
template <>
struct packs<> {
    template <class F, class... X>
    static constexpr void for_each(F&& f, X... x) {
        std::forward<F>(f)(x...);
    }
};

// Pack of packs: several packs
template <class... Args, class... Packs>
struct packs<pack<Args...>, Packs...> {
    template <class F, class... X>
    static constexpr void for_each(F&& f, X... x) {
        for_each(
            std::index_sequence_for<Args...>{},
            std::forward<F>(f),
            x...
        );
    }
    template <std::size_t... I, class F, class... X>
    static constexpr void for_each(std::index_sequence<I...>, F&& f, X... x) {
        ((void)packs<Packs...>::for_each(
            std::forward<F>(f),
            x...,
            pack_element<I, Args>{}
        ), ...);
    }
};

// ========================================================================== //



// ================================ CV STRING =============================== //
// CV-string: default
template <class T, class Length = full>
struct cv_string {
    static constexpr auto value = "";
    static constexpr auto space = "";
    static constexpr bool exists = false;
};

// CV-string: lvalue-reference
template <class T, class Length>
struct cv_string<T&, Length>: cv_string<T, Length> {};

// CV-string: rvalue-reference
template <class T, class Length>
struct cv_string<T&&, Length>: cv_string<T, Length> {};

// CV-string: const
template <class T>
struct cv_string<const T, full> {
    static constexpr auto value = "const";
    static constexpr bool exists = true;
};

// CV-string: volatile
template <class T>
struct cv_string<volatile T, full> {
    static constexpr auto value = "volatile";
    static constexpr bool exists = true;
};

// CV-string: const volatile
template <class T>
struct cv_string<const volatile T, full> {
    static constexpr auto value = "const volatile";
    static constexpr bool exists = true;
};

// CV-string: c
template <class T>
struct cv_string<const T, terse> {
    static constexpr auto value = "c";
    static constexpr bool exists = true;
};

// CV-string: v
template <class T>
struct cv_string<volatile T, terse> {
    static constexpr auto value = "v";
    static constexpr bool exists = true;
};

// CV-string: cv
template <class T>
struct cv_string<const volatile T, terse> {
    static constexpr auto value = "cv";
    static constexpr bool exists = true;
};
// ========================================================================== //



// =============================== REF STRING =============================== //
// Ref-string: default
template <class T, class Length = full>
struct ref_string {
    static constexpr auto value = "";
    static constexpr bool exists = false;
};

// Ref-string: lvalue-reference
template <class T, class Length>
struct ref_string<T&, Length> {
    static constexpr auto value = "&";
    static constexpr bool exists = true;
};

// Ref-string: rvalue-reference
template <class T, class Length>
struct ref_string<T&&, Length> {
    static constexpr auto value = "&&";
    static constexpr bool exists = true;
};
// ========================================================================== //



// ================================ STRINGIFY =============================== //
// Converts a cvref-qualified type to a string
template <class T, class Length = full, class Direction = west>
class stringify
{
    // Types
    public:
    using type = T;
    using string = std::string;
    using cv_string = ::qualificator::cv_string<T, Length>;
    using ref_string = ::qualificator::ref_string<T, Length>;

    // Lifecycle
    public:
    constexpr stringify(const string& name = "T")
    : _value() {
        if (name.empty()) {
            _value += cv_string::value;
            if (cv_string::exists && ref_string::exists) {
                _value += "";
            }
            _value += ref_string::value;
        } else {
            if constexpr (std::is_same_v<Direction, east>) {
                _value += name;
                if (cv_string::exists || ref_string::exists) {
                    _value += " ";
                    _value += cv_string::value;
                    if (cv_string::exists && ref_string::exists) {
                        _value += " ";
                    }
                    _value += ref_string::value;
                }
            } else {
                if (cv_string::exists) {
                    _value += cv_string::value;
                    _value += " ";
                }
                _value += name;
                if (ref_string::exists) {
                    _value += "";
                    _value += ref_string::value;
                }
            }
        }
    }

    // Conversion
    public:
    explicit constexpr operator const string&() const noexcept {
        return _value;
    }

    // Access
    public:
    constexpr const string& operator()() const noexcept {
        return _value;
    }

    // Implementation details: data members
    private:
    string _value;
};

// Streaming
template <class T, class Length, class Direction>
std::ostream& operator<<(
    std::ostream& stream, 
    const stringify<T, Length, Direction>& value
) {
    return stream << value();
}
// ========================================================================== //



// ================================ TEXCOLOR ================================ //
// Texcolor: default
template <class T>
struct texcolor {
    static constexpr auto value = "blue!20";
};

// Texcolor: const
template <class T>
struct texcolor<const T> {
    static constexpr auto value = "blue!40";
};

// Texcolor: volatile
template <class T>
struct texcolor<volatile T> {
    static constexpr auto value = "blue!60";
};

// Texcolor: const volatile
template <class T>
struct texcolor<const volatile T> {
    static constexpr auto value = "blue!80";
};

// Texcolor: lvalue-reference
template <class T>
struct texcolor<T&> {
    static constexpr auto value = "green!20";
};

// Texcolor: const lvalue-reference
template <class T>
struct texcolor<const T&> {
    static constexpr auto value = "green!40";
};

// Texcolor: volatile lvalue-reference
template <class T>
struct texcolor<volatile T&> {
    static constexpr auto value = "green!60";
};

// Texcolor: const volatile lvalue-reference
template <class T>
struct texcolor<const volatile T&> {
    static constexpr auto value = "green!80";
};

// Texcolor: rvalue-reference
template <class T>
struct texcolor<T&&> {
    static constexpr auto value = "red!20";
};

// Texcolor: const rvalue-reference
template <class T>
struct texcolor<const T&&> {
    static constexpr auto value = "red!40";
};

// Texcolor: volatile rvalue-reference
template <class T>
struct texcolor<volatile T&&> {
    static constexpr auto value = "red!60";
};

// Texcolor: const volatile rvalue-reference
template <class T>
struct texcolor<const volatile T&&> {
    static constexpr auto value = "red!80";
};
// ========================================================================== //



// ================================= TEXCELL ================================ //
// Rotates the text in a cell of code
std::string texrotate(int x, const std::string& text) {
    std::string prefix = "\\rotatebox[origin = c]{" + std::to_string(x) + "}";
    return prefix + "{\\hphantom{\\ }" + text + "\\hphantom{\\ }}";
}

// Colors a cell of code
std::string texcellcolor(const std::string& color) {
    return "\\cellcolor{" + color + "}";
}

// Makes a cell of code
std::string texcell(const std::string& code) {
    return code.empty() ? code : "\\lstinline!" + code + "!";
}

// Makes colored cell of code
std::string texcell(const std::string& code, const std::string& color) {
    return texcellcolor(color) + texcell(code);
}

// Makes a rotated cell of code
std::string texcell(int x, const std::string& code) {
    return texrotate(x, texcell(code));
}

// Makes a rotated colored cell of code
std::string texcell(int x, const std::string& code, const std::string& color) {
    return texcellcolor(color) + texrotate(x, texcell(code));
}

// Makes a cell based on a boolean
std::string texcell(bool condition) {
    const std::string color = condition ? "green" : "red";
    const std::string text = condition ? "true" : "false";
    return texcellcolor(color) + texcell(text);
}
// ========================================================================== //



// ================================= TEXIFY ================================= //
// The base of the texify tool
class texify_base
{
    // Types
    public:
    using string = std::string;

    // Conversion
    public:
    explicit constexpr operator const string&() const noexcept {
        return _tex;
    }

    // Access
    public:
    constexpr const string& operator()() const noexcept {
        return _tex;
    }

    // Parts
    public:
    static const std::string& prefix() {
        static const string endl = "\n";
        static const string tex = string{}
        + "\\documentclass{standalone}" + endl
        + "\\usepackage{array}" + endl
        + "\\usepackage{graphicx}" + endl
        + "\\usepackage{listings}" + endl
        + "\\usepackage{tabularx}" + endl
        + "\\usepackage[table]{xcolor}" + endl
        + "\\usepackage[tt = false]{libertine}" + endl
        + "\\lstset{"
        + "showstringspaces = false, "
        + "basicstyle = \\ttfamily, "
        + "breaklines = true"
        + "}"
        + "\\begin{document}" + endl;
        return tex;
    };
    static const std::string& postfix() {
        static const string endl = "\n";
        static const string tex = string{}
        + "\\end{document}" + endl;
        return tex;
    };

    // File
    public:
    bool save(string filename) const {
        std::ofstream texfile;
        texfile.open(filename);
        texfile << _tex;
        texfile.close();
        return texfile.good();
    }
    int make(string filename, string command = "maketex") const {
        const std::string space = " ";
        save(filename);
        command += space + filename;
        return std::system(command.data());
    }

    // Implementation details: data members
    protected:
    string _tex;
};

// Converts the input to a tex document: declaration
template <class... Packs>
struct texify;

// Converts the input to a tex document: one input
template <class... Args>
struct texify<pack<Args...>>: texify_base {
    using packs = ::qualificator::packs<pack<Args...>>;
    template <class F>
    constexpr texify(
        const std::string& name,
        F&& f,
        const std::string& colored = ""
    ): texify_base() {
        using string = typename texify_base::string;
        const string endl = "\n";
        string tex;
        tex += prefix();
        tex += "\\begin{tabular}{|l||c|}" + endl;
        tex += "\\hline" + endl;
        tex += "\\multicolumn{1}{|c||}{" + texcell("decltype(T)") + "} & ";
        tex += texcell(name) + "\\\\ \\hline \\hline";
        packs::for_each([&f, &tex, &endl, &colored](auto x){
            using type = typename decltype(x)::type;
            if (colored.empty()) {
                tex += texcell(stringify<type>{}(), texcolor<type>::value);
            } else {
                tex += texcell(stringify<type>{}(), colored);
            }
            tex += " & ";
            tex += f(x);
            tex += "\\\\ \\hline";
            tex += endl;
        });
        tex += "\\end{tabular}" + endl;
        tex += postfix();
        _tex = tex;
    }
};

// Converts the input to a tex document: two inputs
template <class... Args0, class... Args1>
struct texify<pack<Args0...>, pack<Args1...>>: texify_base {
    using packs = ::qualificator::packs<pack<Args0...>, pack<Args1...>>;
    template <class F>
    constexpr texify(
        const std::string& name,
        F&& f,
        const std::string& colored = ""
    ): texify_base() {
        using string = typename texify_base::string;
        using columns = ::qualificator::packs<pack<Args1...>>;
        using rows = ::qualificator::packs<pack<Args0...>>;
        const string endl = "\n";
        string tex;
        tex += prefix();
        tex += "\\begin{tabular}{|l||";
        columns::for_each([&tex](auto){tex += "c|";});
        tex += "}" + endl;
        tex += "\\hline" + endl;
        tex += "\\multicolumn{1}{|c||}{" + texcell(name) + "}";
        columns::for_each([&tex, &colored](auto x){
            using type = typename decltype(x)::type;
            using color = texcolor<type>;
            if (colored.empty()) {
                tex += " & " + texcell(90, stringify<type>{}(), color::value);
            } else {
                tex += " & " + texcell(90, stringify<type>{}(), colored);
            }
        });
        tex += "\\\\ \\hline \\hline" + endl;
        packs::for_each([&f, &tex, &endl, &colored](auto row, auto column){
            using type = typename decltype(row)::type;
            if (column.index == 0) {
                if (colored.empty()) {
                    tex += texcell(stringify<type>{}(), texcolor<type>::value);
                } else {
                    tex += texcell(stringify<type>{}(), colored);
                }
            }
            tex += " & ";
            tex += f(row, column);
            if (column.index + 1 == sizeof...(Args1)) {
                tex += "\\\\ \\hline";
                tex += endl;
            }
        });
        tex += "\\end{tabular}" + endl;
        tex += postfix();
        _tex = tex;
    }
};

// Streaming
template <class... Packs>
std::ostream& operator<<(
    std::ostream& stream, 
    const texify<Packs...>& value
) {
    return stream << value();
}
// ========================================================================== //




// ========================================================================== //
} // namespace qualificator
#endif // _QUALIFICATOR_HPP_INCLUDED
// ========================================================================== //
