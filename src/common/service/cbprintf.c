#include "service/cbprintf.h"
#include "service/assert.h"
#include <string.h>
#include <stdarg.h>

#define DIGITS_BUFFER_SIZE   21  // 64 bit number with base 10 gives max 20 characters, plus null terminator

enum fspec_flag {
    FLAG_PAD_ZEROES = (1 << 0),
};

enum fspec_length {
    LENGTH_NONE = 0, ///< int
    LENGTH_HH, ///< char
    LENGTH_H, ///< short
    LENGTH_L, ///< long
    LENGTH_LL, ///< long long
    LENGTH_Z, ///< size_t
};

enum fspec_specifier {
    SPECIFIER_NONE = 0,
    SPECIFIER_SIGNED_DEC, ///< 'd' or 'i'
    SPECIFIER_UNSIGNED_DEC, ///< 'u'
    SPECIFIER_UNSIGNED_HEX, ///< 'x'
    SPECIFIER_POINTER, ///< 'p'
    SPECIFIER_STRING, ///< 's'
    SPECIFIER_ESCAPE_PERCENT, ///< '%'
};

enum parse_state {
    PARSE_STATE_REGULAR_CHAR, ///< A regular character was processed.
    PARSE_STATE_FSPEC_PARSING, ///< Format specifier found, feed more characters.
    PARSE_STATE_FSPEC_COMPLETE, ///< Parsing of format specifier is complete.
    PARSE_STATE_FSPEC_ERROR, ///< Invalid or unsupported characters in format specifier found.
};

struct fspec {
    uint32_t flags; ///< Parsed flags of the format specifier.
    uint32_t min_width; ///< Field width of the format specifier.
    enum fspec_length length; ///< Parsed length modifiers of the format specifier.
    enum fspec_specifier specifier; ///< Parsed format specifier type.
};

static const struct fspec fspec_init = {0};

struct parse_context {
    enum parse_state state;
    struct fspec fspec;
};

static const struct parse_context parse_context_init = {0};

union fspec_value {
    uint64_t unsigned_int;
    int64_t signed_int;
    const char *string;
};

struct write_buffer {
    void *buffer;
    size_t size;
    size_t index;
};

struct read_buffer {
    const void *buffer;
    size_t size;
    size_t index;
};

static enum parse_state parse_format(struct parse_context *ctx, const struct fspec **fspec, char c);
static enum parse_state parse_fspec(struct fspec *fspec, char c);

static void print_fspec(cbprintf_out_t out, const struct fspec *fspec, union fspec_value value);
static void print_padding(cbprintf_out_t out, size_t length, bool_t zeroes, char sign);
static void print_string(cbprintf_out_t out, const char *str);

static bool_t buffer_write(struct write_buffer *buffer, const void *data, size_t length);
static bool_t buffer_read(struct read_buffer *buffer, void *data, size_t length);

static size_t encode_uint(char *buffer, uint64_t value, size_t base);
static size_t get_base(enum fspec_specifier specifier);

void cbprintf(cbprintf_out_t out, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    cbvprintf(out, format, ap);

    va_end(ap);
}

void cbvprintf(cbprintf_out_t out, const char *format, va_list ap)
{
    struct parse_context ctx = parse_context_init;

    for (size_t i = 0; format[i] != '\0'; i++) {
        const struct fspec *fspec = NULL;
        enum parse_state state = parse_format(&ctx, &fspec, format[i]);

        // regular characters are directly printed
        if (state == PARSE_STATE_REGULAR_CHAR) {
            out(format[i]);
            continue;
        }

        // format specifier is being parsed -> continue
        if (state == PARSE_STATE_FSPEC_PARSING) {
            continue;
        }

        // format specifier parsing is complete -> retrieve argument and print it
        if (state == PARSE_STATE_FSPEC_COMPLETE) {
            union fspec_value value = {0};

            // the following block cannot be extracted to a helper function because a `va_list` may become invalid
            // after it is accessed via `va_arg` and the function returns
            switch (fspec->specifier) {
                case SPECIFIER_SIGNED_DEC:
                    switch (fspec->length) {
                        case LENGTH_NONE:
                        case LENGTH_HH: // char is promoted to int when passed to variadic function
                        case LENGTH_H: // short is promoted to int when passed to variadic function
                            value.signed_int = va_arg(ap, int);
                            break;
                        case LENGTH_L:
                            value.signed_int = va_arg(ap, long);
                            break;
                        case LENGTH_LL:
                            value.signed_int = va_arg(ap, long long);
                            break;
                        case LENGTH_Z:
                            value.signed_int = va_arg(ap, size_t);
                            break;
                        default:
                            RUNTIME_ASSERT(false);
                            break;
                    }
                    break;
                case SPECIFIER_UNSIGNED_DEC:
                case SPECIFIER_UNSIGNED_HEX:
                    switch (fspec->length) {
                        case LENGTH_NONE:
                        case LENGTH_HH: // char is promoted to int when passed to variadic function
                        case LENGTH_H: // short is promoted to int when passed to variadic function
                            value.unsigned_int = va_arg(ap, unsigned int);
                            break;
                        case LENGTH_L:
                            value.unsigned_int = va_arg(ap, unsigned long);
                            break;
                        case LENGTH_LL:
                            value.unsigned_int = va_arg(ap, unsigned long long);
                            break;
                        case LENGTH_Z:
                            value.unsigned_int = va_arg(ap, size_t);
                            break;
                        default:
                            RUNTIME_ASSERT(false);
                            break;
                    }
                    break;
                case SPECIFIER_POINTER:
                    value.unsigned_int = (uintptr_t) va_arg(ap, void*);
                    break;
                case SPECIFIER_STRING:
                    value.string = va_arg(ap, const char*);
                    break;
                case SPECIFIER_ESCAPE_PERCENT:
                    // do nothing
                    break;
                default:
                    RUNTIME_ASSERT(false);
                    break;
            }

            print_fspec(out, fspec, value);
            continue;
        }

        // if a format specifier could not be parsed, we abort processing
        if (state == PARSE_STATE_FSPEC_ERROR) {
            break;
        }

        // unhandled state
        RUNTIME_ASSERT(false);
        break;
    }
}

size_t cbprintf_capture(void *packaged, size_t length, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);

    size_t ret = cbvprintf_capture(packaged, length, format, ap);

    va_end(ap);

    return ret;
}

size_t cbvprintf_capture(void *packaged, size_t length, const char *format, va_list ap)
{
    // first store the format string itself
    struct write_buffer buffer = {
        .buffer = packaged,
        .size = length,
        .index = 0,
    };

    if (!buffer_write(&buffer, &format, sizeof(format))) {
        return 0;
    }

    // then parse through the format string and store all contained arguments
    struct parse_context ctx = parse_context_init;

    for (size_t i = 0; format[i] != '\0'; i++) {
        const struct fspec *fspec = NULL;
        enum parse_state state = parse_format(&ctx, &fspec, format[i]);

        // regular characters are ignored
        if (state == PARSE_STATE_REGULAR_CHAR) {
            continue;
        }

        // format specifier is being parsed -> continue
        if (state == PARSE_STATE_FSPEC_PARSING) {
            continue;
        }

        // format specifier parsing is complete -> retrieve argument and store it
        if (state == PARSE_STATE_FSPEC_COMPLETE) {
            // the following block cannot be extracted to a helper function because a `va_list` may become invalid
            // after it is accessed via `va_arg` and the function returns
            switch (fspec->specifier) {
                case SPECIFIER_SIGNED_DEC:
                    switch (fspec->length) {
                        case LENGTH_NONE: {
                            int value = va_arg(ap, int);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_HH: {
                            // char is promoted to int when passed to variadic function
                            char value = (char) va_arg(ap, int);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_H: {
                            // short is promoted to int when passed to variadic function
                            short value = (short) va_arg(ap, int);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_L: {
                            long value = va_arg(ap, long);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_LL: {
                            long long value = va_arg(ap, long long);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_Z: {
                            size_t value = va_arg(ap, size_t);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        default: {
                            RUNTIME_ASSERT(false);
                            break;
                        }
                    }
                    break;
                case SPECIFIER_UNSIGNED_DEC:
                case SPECIFIER_UNSIGNED_HEX:
                    switch (fspec->length) {
                        case LENGTH_NONE: {
                            int value = va_arg(ap, int);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_HH: {
                            // char is promoted to int when passed to variadic function
                            char value = (char) va_arg(ap, int);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_H: {
                            // short is promoted to int when passed to variadic function
                            short value = (short) va_arg(ap, int);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_L: {
                            long value = va_arg(ap, long);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_LL: {
                            long long value = va_arg(ap, long long);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        case LENGTH_Z: {
                            size_t value = va_arg(ap, size_t);
                            buffer_write(&buffer, &value, sizeof(value));
                            break;
                        }
                        default: {
                            RUNTIME_ASSERT(false);
                            break;
                        }
                    }
                    break;
                case SPECIFIER_POINTER: {
                    uintptr_t value = (uintptr_t) va_arg(ap, void*);
                    buffer_write(&buffer, &value, sizeof(value));
                    break;
                }
                case SPECIFIER_STRING: {
                    const char *value = va_arg(ap, const char*);
                    buffer_write(&buffer, &value, sizeof(value));
                    break;
                }
                case SPECIFIER_ESCAPE_PERCENT: {
                    // do nothing
                    break;
                }
                default: {
                    RUNTIME_ASSERT(false);
                    break;
                }
            }

            continue;
        }

        // if a format specifier could not be parsed, we discard any data and abort processing
        if (state == PARSE_STATE_FSPEC_ERROR) {
            buffer.index = 0;
            break;
        }

        // unhandled state
        RUNTIME_ASSERT(false);
        break;
    }

    return buffer.index;
}

void cbprintf_restore(cbprintf_out_t out, const void *packaged, size_t length)
{
    // first retrieve the format string itself
    struct read_buffer buffer = {
        .buffer = packaged,
        .size = length,
        .index = 0,
    };

    const char *format = NULL;

    if (!buffer_read(&buffer, &format, sizeof(format))) {
        // cannot happen unless the passed data is corrupt or incomplete
        RUNTIME_ASSERT(false);
        return;
    }

    // then parse through the format string and retrieve all contained arguments
    struct parse_context ctx = parse_context_init;

    for (size_t i = 0; format[i] != '\0'; i++) {
        const struct fspec *fspec = NULL;
        enum parse_state state = parse_format(&ctx, &fspec, format[i]);

        // regular characters are directly printed
        if (state == PARSE_STATE_REGULAR_CHAR) {
            out(format[i]);
            continue;
        }

        // format specifier is being parsed -> continue
        if (state == PARSE_STATE_FSPEC_PARSING) {
            continue;
        }

        // format specifier parsing is complete -> read value and print it
        if (state == PARSE_STATE_FSPEC_COMPLETE) {
            union fspec_value value = {0};
            bool_t value_read = false;

            // the following block is not extracted to a helper function to stay consistent with cbprintf_capture
            switch (fspec->specifier) {
                case SPECIFIER_SIGNED_DEC: {
                    switch (fspec->length) {
                        case LENGTH_NONE: {
                            int temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.signed_int = temp;
                            break;
                        }
                        case LENGTH_HH: {
                            char temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.signed_int = temp;
                            break;
                        }
                        case LENGTH_H: {
                            short temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.signed_int = temp;
                            break;
                        }
                        case LENGTH_L: {
                            long temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.signed_int = temp;
                            break;
                        }
                        case LENGTH_LL: {
                            long long temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.signed_int = temp;
                            break;
                        }
                        case LENGTH_Z: {
                            size_t temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.signed_int = temp;
                            break;
                        }
                        default: {
                            RUNTIME_ASSERT(false);
                            break;
                        }
                    }
                    break;
                }
                case SPECIFIER_UNSIGNED_DEC:
                case SPECIFIER_UNSIGNED_HEX: {
                    switch (fspec->length) {
                        case LENGTH_NONE: {
                            unsigned int temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.unsigned_int = temp;
                            break;
                        }
                        case LENGTH_HH: {
                            unsigned char temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.unsigned_int = temp;
                            break;
                        }
                        case LENGTH_H: {
                            unsigned short temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.unsigned_int = temp;
                            break;
                        }
                        case LENGTH_L: {
                            unsigned long temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.unsigned_int = temp;
                            break;
                        }
                        case LENGTH_LL: {
                            unsigned long long temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.unsigned_int = temp;
                            break;
                        }
                        case LENGTH_Z: {
                            size_t temp = 0;
                            value_read = buffer_read(&buffer, &temp, sizeof(temp));
                            value.unsigned_int = temp;
                            break;
                        }
                        default: {
                            RUNTIME_ASSERT(false);
                            break;
                        }
                    }
                    break;
                }
                case SPECIFIER_POINTER: {
                    uintptr_t temp = 0;
                    value_read = buffer_read(&buffer, &temp, sizeof(temp));
                    value.unsigned_int = temp;
                    break;
                }
                case SPECIFIER_STRING: {
                    const char *temp = 0;
                    value_read = buffer_read(&buffer, &temp, sizeof(temp));
                    value.string = temp;
                    break;
                }
                case SPECIFIER_ESCAPE_PERCENT: {
                    value_read = true;
                    break;
                }
                default: {
                    RUNTIME_ASSERT(false);
                    break;
                }
            }

            if (!value_read) {
                // cannot happen unless the passed data is corrupt or incomplete
                RUNTIME_ASSERT(false);
                break;
            }

            print_fspec(out, fspec, value);
            continue;
        }

        // unhandled state
        // parsing error has to occur already when capturing
        RUNTIME_ASSERT(false);
        break;
    }
}

/**
 * Helper function to parse a format string.
 *
 * This function is supposed to be called repeatedly for each character in a format string.
 * If the function returns `PARSE_STATE_FSPEC_COMPLETE`, `fspec` is set to the parsed format specifier.
 * If the function returns `PARSE_STATE_FSPEC_ERROR`, parsing must be aborted.
 *
 * @param ctx Parser context.
 * @param fspec Pointer to the parsed format specifier.
 * @param c Next character to process.
 * @return State of the operation.
 */
static enum parse_state parse_format(struct parse_context *ctx, const struct fspec **fspec, char c)
{
    *fspec = NULL;

    switch (ctx->state) {
        case PARSE_STATE_REGULAR_CHAR: {
            if (c == '%') {
                ctx->state = PARSE_STATE_FSPEC_PARSING;
                ctx->fspec = fspec_init;
            }
            break;
        }
        case PARSE_STATE_FSPEC_PARSING: {
            ctx->state = parse_fspec(&ctx->fspec, c);

            if (ctx->state == PARSE_STATE_FSPEC_COMPLETE) {
                *fspec = &ctx->fspec;
            }
            break;
        }
        case PARSE_STATE_FSPEC_COMPLETE: {
            if (c == '%') {
                ctx->state = PARSE_STATE_FSPEC_PARSING;
                ctx->fspec = fspec_init;
            } else {
                ctx->state = PARSE_STATE_REGULAR_CHAR;
            }
            break;
        }
        case PARSE_STATE_FSPEC_ERROR: {
            // parsing must not continue after a parsing error
            RUNTIME_ASSERT(false);
            break;
        }
    }

    return ctx->state;
}

static enum parse_state parse_fspec(struct fspec *fspec, char c)
{
    enum parse_state state = PARSE_STATE_FSPEC_PARSING;

    switch (c) {
        case '0': {
            if (fspec->min_width == 0) {
                fspec->flags |= FLAG_PAD_ZEROES;
            } else {
                fspec->min_width *= 10;
            }
            break;
        }
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            fspec->min_width = (10 * fspec->min_width) + (c - '0');
            break;
        }
        case 'h': {
            if (fspec->length == LENGTH_H) {
                fspec->length = LENGTH_HH;
            } else {
                fspec->length = LENGTH_H;
            }
            break;
        }
        case 'l': {
            if (fspec->length == LENGTH_L) {
                fspec->length = LENGTH_LL;
            } else {
                fspec->length = LENGTH_L;
            }
            break;
        }
        case 'z': {
            fspec->length = LENGTH_Z;
            break;
        }
        case 'd':
        case 'i': {
            fspec->specifier = SPECIFIER_SIGNED_DEC;
            state = PARSE_STATE_FSPEC_COMPLETE;
            break;
        }
        case 'u': {
            fspec->specifier = SPECIFIER_UNSIGNED_DEC;
            state = PARSE_STATE_FSPEC_COMPLETE;
            break;
        }
        case 'x': {
            fspec->specifier = SPECIFIER_UNSIGNED_HEX;
            state = PARSE_STATE_FSPEC_COMPLETE;
            break;
        }
        case 'p': {
            fspec->specifier = SPECIFIER_POINTER;
            state = PARSE_STATE_FSPEC_COMPLETE;
            break;
        }
        case 's': {
            fspec->specifier = SPECIFIER_STRING;
            state = PARSE_STATE_FSPEC_COMPLETE;
            break;
        }
        default: {
            state = PARSE_STATE_FSPEC_ERROR;
            break;
        }
    }

    return state;
}

/**
 * Helper function to print a format specifier value using a provided print function.
 *
 * @param out Function to print characters.
 * @param fspec Information about the value to print.
 * @param value Value to print.
 */
static void print_fspec(cbprintf_out_t out, const struct fspec *fspec, union fspec_value value)
{
    switch (fspec->specifier) {
        case SPECIFIER_SIGNED_DEC: {
            bool_t is_negative = (value.signed_int < 0);
            uint64_t value_abs = is_negative ? -value.signed_int : value.signed_int;

            char digits[DIGITS_BUFFER_SIZE];
            size_t num_digits = encode_uint(digits, value_abs, get_base(fspec->specifier));

            size_t pad_length = num_digits < fspec->min_width ? fspec->min_width - num_digits : 0;
            bool_t pad_zeroes = (fspec->flags & FLAG_PAD_ZEROES) != 0;
            char pad_char = is_negative ? '-' : '\0';

            print_padding(out, pad_length, pad_zeroes, pad_char);
            print_string(out, digits);
            break;
        }
        case SPECIFIER_UNSIGNED_DEC:
        case SPECIFIER_UNSIGNED_HEX:
        case SPECIFIER_POINTER: {
            char digits[DIGITS_BUFFER_SIZE];
            size_t num_digits = encode_uint(digits, value.unsigned_int, get_base(fspec->specifier));

            size_t pad_length = num_digits < fspec->min_width ? fspec->min_width - num_digits : 0;
            bool_t pad_zeroes = (fspec->flags & FLAG_PAD_ZEROES) != 0;

            print_padding(out, pad_length, pad_zeroes, '\0');
            print_string(out, digits);
            break;
        }
        case SPECIFIER_STRING: {
            print_string(out, value.string);
            break;
        }
        case SPECIFIER_ESCAPE_PERCENT: {
            out('%');
            break;
        }
        default: {
            RUNTIME_ASSERT(false);
            break;
        }
    }
}

/**
 * Writes arbitrary data to the provided `buffer`.
 *
 * @param buffer Package to write to.
 * @param data Pointer to the data to write.
 * @param length Number of bytes to write.
 * @return True on success, false if there was not enough space.
 */
bool_t buffer_write(struct write_buffer *buffer, const void *data, size_t length)
{
    if (buffer->index + length > buffer->size) {
        return false;
    }

    memcpy((uint8_t *) buffer->buffer + buffer->index, data, length);
    buffer->index += length;
    return true;
}

/**
 * Reads arbitrary data from the provided `buffer`.
 *
 * @param buffer Package to read from.
 * @param data Pointer where to store the read data.
 * @param length Number of bytes to read.
 * @return True on success, false if there was not enough data in the package.
 */
bool_t buffer_read(struct read_buffer *buffer, void *data, size_t length)
{
    if (buffer->index + length > buffer->size) {
        return false;
    }

    memcpy(data, (uint8_t *) buffer->buffer + buffer->index, length);
    buffer->index += length;
    return true;
}

/**
 * Prints a padding for a right justified field.
 *
 * @param out Output function.
 * @param length Length of the padding.
 * @param zeroes True to pad with '0', false to pad with ' '.
 * @param sign Optional sign for the field. Should be '+', '-', or '\0'. Always printed.
 */
static void print_padding(cbprintf_out_t out, size_t length, bool_t zeroes, char sign)
{
    if (zeroes && (sign != '\0')) {
        out(sign);
    }

    if ((sign != '\0') && (length > 0)) {
        length--;
    }

    char pad_char = zeroes ? '0' : ' ';

    for (size_t i = 0; i < length; i++) {
        out(pad_char);
    }

    if (!zeroes && (sign != '\0')) {
        out(sign);
    }
}

/**
 * Prints a null terminated string using the provided output function.
 *
 * @param out Output function.
 * @param str Null terminated string to print.
 */
static void print_string(cbprintf_out_t out, const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++) {
        out(str[i]);
    }
}

/**
 * Encodes a unsigned integer value into a string buffer.
 *
 * The string is null terminated.
 *
 * @param buffer Buffer to write the encoded value into. Should be `DIGITS_BUFFER_SIZE` in size.
 * @param value Value to encode.
 * @param base Number base to use (should be 10 or 16).
 * @return Number of characters written, excluding the null terminator.
 */
static size_t encode_uint(char *buffer, uint64_t value, size_t base)
{
    // handle case where value is zero
    if (value == 0) {
        buffer[0] = '0';
        buffer[1] = '\0';
        return 1;
    }

    // write value to buffer (in reverse order)
    int write_idx = 0;

    while (value > 0) {
        uint8_t c = (uint8_t) (value % base);

        if (c >= 10) {
            c = (uint8_t) 'a' + c - 10;
        } else {
            c = (uint8_t) '0' + c;
        }

        buffer[write_idx] = (char) c;
        write_idx++;

        value /= base;
    }

    // reverse data in buffer
    int start = 0;
    int end = write_idx - 1;

    while (start < end) {
        char temp = buffer[start];
        buffer[start] = buffer[end];
        buffer[end] = temp;

        start++;
        end--;
    }

    // null terminator
    buffer[write_idx] = '\0';

    return write_idx;
}

/**
 * Returns the number base for the given specifier.
 *
 * @param specifier Format specifier.
 * @return Number base.
 */
static size_t get_base(enum fspec_specifier specifier)
{
    switch (specifier) {
        case SPECIFIER_SIGNED_DEC:
        case SPECIFIER_UNSIGNED_DEC:
            return 10;
        case SPECIFIER_UNSIGNED_HEX:
        case SPECIFIER_POINTER:
            return 16;
        default:
            RUNTIME_ASSERT(false);
            return 0;
    }
}
