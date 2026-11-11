/* Salap core, C interface. For Swift and anything else that cannot include
 * the C++ header. All strings are NUL-terminated UTF-8. Every returned string
 * is newly allocated and must be released with salap_free(). */
#ifndef SALAP_C_H
#define SALAP_C_H

#ifdef __cplusplus
extern "C" {
#endif

/* direction: 0 = detect, 1 = English keys to Thai, 2 = Thai keys to English */
char* salap_convert_layout(const char* text, int direction);

/* direction: 0 = auto, 1 = to Thai digits, 2 = to Arabic digits */
char* salap_convert_numerals(const char* text, int direction);

/* Returns NULL if `number` is not a number. */
char* salap_baht_text(const char* number);

/* `converted` may be NULL. */
char* salap_convert_years(const char* text, int* converted);

/* 0 = nothing, 1 = fix language, 2 = numerals */
int salap_smart_action(const char* text);

void salap_free(char* s);

#ifdef __cplusplus
}
#endif

#endif
