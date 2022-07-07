#pragma once

using namespace std;

void trim(string& s);
void remove_space(string& s);
bool stob(string const &s);

void string_replace(string &haystack, string const &pattern, string const &replacement);
bool string_contains(string const &haystack, string const &pattern);
bool string_contains(string const &haystack, char const *pattern);
bool string_has_prefix(string const &s, string const &expected_prefix);
bool string_has_suffix(string const &s, string const &expected_suffix);
bool is_line(const string& line, const string& keyword);
bool string_is_numeric(string const &str);
bool stringIsInteger(string const &s);
bool stringStartsWith(string const &haystack, string const &needle);
string string_get_maximal_alpha_prefix(string const& s);
string lowercase(string const& s);

string pad_string(string const &in, size_t padlen);
string lpad_string(string const &in, size_t padlen);

