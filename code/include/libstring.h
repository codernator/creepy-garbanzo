void smash_tilde(char *str);
bool str_cmp(const char *astr, const char *bstr);
bool str_prefix(const char *astr, const char *bstr);
bool str_infix(const char *astr, const char *bstr);
bool str_suffix(const char *astr, const char *bstr);
char *str_replace(char *orig, char *find, char *replace);
char *capitalize(const char *str);
void capitalize_into(const char *source, /*@out@*/ char *initialized_target, size_t string_length);
bool is_space(const char test);
bool is_digit(const char test);
bool is_alpha(const char test);
bool is_upper(const char test);
bool is_alnum(const char test);
bool is_number(const char *test);
char *lower(char *s);

