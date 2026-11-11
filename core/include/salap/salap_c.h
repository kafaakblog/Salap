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

/* Returns NULL if `words` is not an amount as salap_baht_text writes it. */
char* salap_parse_baht_text(const char* words);

/* Conversions the shared shortcut should offer for `text`, in order.
 * `features` is a bit mask: 1 fix language, 2 numerals, 4 years.
 * Returns the number of results. Get each with salap_cycle_item (index from
 * 0), then release the list with salap_cycle_free. */
typedef struct salap_cycle salap_cycle;
salap_cycle* salap_conversion_cycle(const char* text, unsigned features);
int salap_cycle_count(const salap_cycle* c);
const char* salap_cycle_item(const salap_cycle* c, int index); /* owned by the list */
void salap_cycle_free(salap_cycle* c);

void salap_free(char* s);

#ifdef __cplusplus
}
#endif

#endif
