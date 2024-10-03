#pragma once

#include <util/types.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*cbprintf_out_t)(char c);

/**
 * Formats a string outputting it character wise to the provided output function.
 *
 * The following printf features are supported:
 *
 * - Specifiers: d, i, u, x, s, p
 * - Length: hh, h, l, ll, z
 * - Width: Only for numeric fields, asterisk is not supported
 * - Precision: Not supported
 * - Flags: 0
 *
 * @param out Function to output characaters.
 * @param format Format string.
 * @param ... Arguments for the format string.
 */
void cbprintf(cbprintf_out_t out, const char *format, ...);

/**
 * Similar to `cbprintf` but taking a `va_list`.
 */
void cbvprintf(cbprintf_out_t out, const char *format, va_list ap);

/**
 * Captures the provided format string together with its arguments and stores it in the provided buffer
 * `packagted`. No formatting is done yet. The string can then be formatted later using `cbprintf_restore`.
 * For supported format string features see the `cbprintf` function.
 *
 * @warning The format string and any string arguments are not copied. Their memory must still be valid if
 *          the string is formatted using `cbprintf_restore`.
 *
 * @param packaged Buffer to store the captured data.
 * @param length Number of bytes available in the buffer.
 * @param format Format string.
 * @param ... Arguments for the format string.
 * @return Number of bytes written to the buffer or zero if there was an error.
 */
size_t cbprintf_capture(void *packaged, size_t length, const char *format, ...);

/**
 * Similar to `cbprintf_capture` but taking a `va_list`.
 */
size_t cbvprintf_capture(void *packaged, size_t length, const char *format, va_list ap);

/**
 * Restores a previously captured string by formatting it and outputting it character wise to the provided
 * output function. See `cbprintf_capture`.
 *
 * @param out Function to output characters.
 * @param packaged Buffer containing the captured string.
 * @param length Number of bytes in the buffer.
 */
void cbprintf_restore(cbprintf_out_t out, const void *packaged, size_t length);

#ifdef __cplusplus
}
#endif
