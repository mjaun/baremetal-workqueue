#include "service/cbprintf.h"
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

enum fspec_parse_state {
    PARSE_STATE_CONTINUE, ///< Parsing of format specifier is ongoing, feed more characters.
    PARSE_STATE_COMPLETE, ///< Parsing of format specifier is complete.
    PARSE_STATE_ERROR, ///< Invalid or unsupported characters found.
};

struct fspec {
    uint32_t flags; ///< Parsed flags of the format specifier.
    uint32_t min_width; ///< Field width of the format specifier.
    enum fspec_length length; ///< Parsed length modifiers of the format specifier.
    enum fspec_specifier specifier; ///< Parsed format specifier type.

    char _prev_char; ///< Internal variable to remember the last character.
};

static const struct fspec fspec_init = {
    .flags = 0,
    .min_width = 0,
    .length = LENGTH_NONE,
    .specifier = SPECIFIER_NONE,
    ._prev_char = '\0',
};

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

static enum fspec_parse_state fspec_parse(struct fspec *fspec, char c);
static bool_t fspec_get(const struct fspec *fspec, union fspec_value *fspec_value, va_list *ap);
static bool_t fspec_pack(const struct fspec *fspec, struct write_buffer *buffer, va_list *ap);
static bool_t fspec_unpack(const struct fspec *fspec, struct read_buffer *buffer, union fspec_value *fspec_value);
static void fspec_print(cbprintf_out_t out, const struct fspec *fspec, union fspec_value value);

static bool_t buffer_write(struct write_buffer *buffer, const void *data, size_t length);
static bool_t buffer_read(struct read_buffer *buffer, void *data, size_t length);

static void print_padding(cbprintf_out_t out, size_t length, bool_t zeroes, char sign);
static void print_string(cbprintf_out_t out, const char *str);

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
    struct fspec fspec = fspec_init;
    bool_t parsing = false;

    for (size_t i = 0; format[i] != '\0'; i++) {
        if (!parsing) {
            // print characters until we reach the beginning of a format specifier
            if (format[i] == '%') {
                fspec = fspec_init;
                parsing = true;
            } else {
                out(format[i]);
            }
        } else {
            // pass characters to parser until complete
            enum fspec_parse_state state = fspec_parse(&fspec, format[i]);

            if (state == PARSE_STATE_ERROR) {
                break;
            }

            // retrieve argument and print value
            if (state == PARSE_STATE_COMPLETE) {
                union fspec_value value;

                if (!fspec_get(&fspec, &value, &ap)) {
                    break;
                }

                fspec_print(out, &fspec, value);
                parsing = false;
            }
        }
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
    struct fspec fspec = fspec_init;
    bool_t parsing = false;

    for (size_t i = 0; format[i] != '\0'; i++) {
        if (!parsing) {
            // skip characters until we reach the beginning of a format specifier
            if (format[i] == '%') {
                fspec = fspec_init;
                parsing = true;
            }
        } else {
            // pass characters to parser until complete
            enum fspec_parse_state state = fspec_parse(&fspec, format[i]);

            if (state == PARSE_STATE_ERROR) {
                buffer.index = 0;
                break;
            }

            // pack argument into buffer
            if (state == PARSE_STATE_COMPLETE) {
                if (!fspec_pack(&fspec, &buffer, &ap)) {
                    buffer.index = 0;
                    break;
                }

                parsing = false;
            }
        }
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
        return;
    }

    // then parse through the format string and retrieve all contained arguments
    struct fspec fspec = fspec_init;
    bool_t parsing = false;

    for (size_t i = 0; format[i] != '\0'; i++) {
        if (!parsing) {
            // print characters until we reach the beginning of a format specifier
            if (format[i] == '%') {
                fspec = fspec_init;
                parsing = true;
            } else {
                out(format[i]);
            }
        } else {
            // pass characters to parser until complete
            enum fspec_parse_state state = fspec_parse(&fspec, format[i]);

            if (state == PARSE_STATE_ERROR) {
                buffer.index = 0;
                break;
            }

            // read argument from buffer and print value
            if (state == PARSE_STATE_COMPLETE) {
                union fspec_value value;

                if (!fspec_unpack(&fspec, &buffer, &value)) {
                    break;
                }

                fspec_print(out, &fspec, value);
                parsing = false;
            }
        }
    }
}

/**
 * Helper function to parse a format specifier. Initialize a new context with `fspec_init` then call this function
 * repeatedly for each character until either `PARSE_STATE_COMPLETE` or `PARSE_STATE_ERROR` is returned.
 *
 * @param fspec Format specifier continously filled with information.
 * @param c Next character to process.
 * @return State of the operation.
 */
static enum fspec_parse_state fspec_parse(struct fspec *fspec, char c)
{
    enum fspec_parse_state state = PARSE_STATE_CONTINUE;

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
            if (fspec->_prev_char == 'h') {
                fspec->length = LENGTH_HH;
            } else {
                fspec->length = LENGTH_H;
            }
            break;
        }
        case 'l': {
            if (fspec->_prev_char == 'l') {
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
            state = PARSE_STATE_COMPLETE;
            break;
        }
        case 'u': {
            fspec->specifier = SPECIFIER_UNSIGNED_DEC;
            state = PARSE_STATE_COMPLETE;
            break;
        }
        case 'x': {
            fspec->specifier = SPECIFIER_UNSIGNED_HEX;
            state = PARSE_STATE_COMPLETE;
            break;
        }
        case 'p': {
            fspec->specifier = SPECIFIER_POINTER;
            state = PARSE_STATE_COMPLETE;
            break;
        }
        case 's': {
            fspec->specifier = SPECIFIER_STRING;
            state = PARSE_STATE_COMPLETE;
            break;
        }
        default: {
            state = PARSE_STATE_ERROR;
            break;
        }
    }

    fspec->_prev_char = c;
    return state;
}

/**
 * Helper function to retrieve a value from a `va_list` based on a provided format specifier.
 *
 * @param fspec Format specifier.
 * @param fspec_value Pointer to store the retrieved value.
 * @param ap Argument list to get the value from.
 * @return True on success, false if there was an error.
 */
bool_t fspec_get(const struct fspec *fspec, union fspec_value *fspec_value, va_list *ap)
{
    bool_t ret = false;

    switch (fspec->specifier) {
        case SPECIFIER_SIGNED_DEC: {
            switch (fspec->length) {
                case LENGTH_NONE: {
                    int value = va_arg(*ap, int);
                    fspec_value->signed_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_HH: {
                    char value = (char) va_arg(*ap, int);
                    fspec_value->signed_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_H: {
                    short value = (short) va_arg(*ap, int);
                    fspec_value->signed_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_L: {
                    long value = va_arg(*ap, long);
                    fspec_value->signed_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_LL: {
                    long long value = va_arg(*ap, long long);
                    fspec_value->signed_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_Z: {
                    size_t value = va_arg(*ap, size_t);
                    fspec_value->signed_int = value;
                    ret = true;
                    break;
                }
            }
            break;
        }
        case SPECIFIER_UNSIGNED_DEC:
        case SPECIFIER_UNSIGNED_HEX: {
            switch (fspec->length) {
                case LENGTH_NONE: {
                    unsigned int value = va_arg(*ap, unsigned int);
                    fspec_value->unsigned_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_HH: {
                    unsigned char value = (unsigned char) va_arg(*ap, int);
                    fspec_value->unsigned_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_H: {
                    unsigned short value = (unsigned short) va_arg(*ap, int);
                    fspec_value->unsigned_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_L: {
                    unsigned long value = va_arg(*ap, unsigned long);
                    fspec_value->unsigned_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_LL: {
                    unsigned long long value = va_arg(*ap, unsigned long long);
                    fspec_value->unsigned_int = value;
                    ret = true;
                    break;
                }
                case LENGTH_Z: {
                    size_t value = va_arg(*ap, size_t);
                    fspec_value->unsigned_int = value;
                    ret = true;
                    break;
                }
            }
            break;
        }
        case SPECIFIER_POINTER: {
            uintptr_t value = (uintptr_t) va_arg(*ap, void*);
            fspec_value->unsigned_int = value;
            ret = true;
            break;
        }
        case SPECIFIER_STRING: {
            const char *value = va_arg(*ap, const char*);
            fspec_value->string = value;
            ret = true;
            break;
        }
        case SPECIFIER_ESCAPE_PERCENT: {
            ret = true;
            break;
        }
        default: {
            ret = false;
            break;
        }
    }

    return ret;
}

/**
 * Helper function to retrieve a value from a `va_list` based on a provided format specifier and pack it into
 * the provided buffer.
 *
 * @param fspec Format specifier.
 * @param buffer Buffer to pack the value into.
 * @param ap Argument list to get the value from.
 * @return True on success, false if there was an error.
 */
static bool_t fspec_pack(const struct fspec *fspec, struct write_buffer *buffer, va_list *ap)
{
    bool_t ret = false;

    switch (fspec->specifier) {
        case SPECIFIER_SIGNED_DEC: {
            switch (fspec->length) {
                case LENGTH_NONE: {
                    int value = va_arg(*ap, int);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_HH: {
                    char value = (char) va_arg(*ap, int);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_H: {
                    short value = (short) va_arg(*ap, int);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_L: {
                    long value = va_arg(*ap, long);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_LL: {
                    long long value = va_arg(*ap, long long);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_Z: {
                    size_t value = va_arg(*ap, size_t);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
            }
            break;
        }
        case SPECIFIER_UNSIGNED_DEC:
        case SPECIFIER_UNSIGNED_HEX: {
            switch (fspec->length) {
                case LENGTH_NONE: {
                    unsigned int value = va_arg(*ap, unsigned int);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_HH: {
                    unsigned char value = (unsigned char) va_arg(*ap, int);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_H: {
                    unsigned short value = (unsigned short) va_arg(*ap, int);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_L: {
                    unsigned long value = va_arg(*ap, unsigned long);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_LL: {
                    unsigned long long value = va_arg(*ap, unsigned long long);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
                case LENGTH_Z: {
                    size_t value = va_arg(*ap, size_t);
                    ret = buffer_write(buffer, &value, sizeof(value));
                    break;
                }
            }
            break;
        }
        case SPECIFIER_POINTER: {
            uintptr_t value = (uintptr_t) va_arg(*ap, void*);
            ret = buffer_write(buffer, &value, sizeof(value));
            break;
        }
        case SPECIFIER_STRING: {
            const char *value = va_arg(*ap, const char*);
            ret = buffer_write(buffer, &value, sizeof(value));
            break;
        }
        case SPECIFIER_ESCAPE_PERCENT: {
            ret = true;
            break;
        }
        default: {
            ret = false;
            break;
        }
    }

    return ret;
}

/**
 * Helper function to unpack a value from a buffer based on a provided format specifier.
 *
 * @param fspec Format specifier.
 * @param buffer Buffer to unpack the value from.
 * @param fspec_value Pointer to store the unpacked value.
 * @return True on success, false if there was an error.
 */
static bool_t fspec_unpack(const struct fspec *fspec, struct read_buffer *buffer, union fspec_value *fspec_value)
{
    bool ret = false;

    switch (fspec->specifier) {
        case SPECIFIER_SIGNED_DEC: {
            switch (fspec->length) {
                case LENGTH_NONE: {
                    int value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_HH: {
                    char value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_H: {
                    short value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_L: {
                    long value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_LL: {
                    long long value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_Z: {
                    size_t value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
            }
            break;
        }
        case SPECIFIER_UNSIGNED_DEC:
        case SPECIFIER_UNSIGNED_HEX: {
            switch (fspec->length) {
                case LENGTH_NONE: {
                    unsigned int value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_HH: {
                    unsigned char value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_H: {
                    unsigned short value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_L: {
                    unsigned long value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_LL: {
                    unsigned long long value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
                case LENGTH_Z: {
                    size_t value = 0;
                    ret = buffer_read(buffer, &value, sizeof(value));
                    fspec_value->signed_int = value;
                    break;
                }
            }
            break;
        }
        case SPECIFIER_POINTER: {
            uintptr_t value = 0;
            ret = buffer_read(buffer, &value, sizeof(value));
            fspec_value->signed_int = value;
            break;
        }
        case SPECIFIER_STRING: {
            const char *value = 0;
            ret = buffer_read(buffer, &value, sizeof(value));
            fspec_value->string = value;
            break;
        }
        case SPECIFIER_ESCAPE_PERCENT: {
            ret = true;
            break;
        }
        default: {
            ret = false;
            break;
        }
    }

    return ret;
}

/**
 * Helper function to print a format specifier value using a provided print function.
 *
 * @param out Function to print characters.
 * @param fspec Information about the value to print.
 * @param value Value to print.
 */
static void fspec_print(cbprintf_out_t out, const struct fspec *fspec, union fspec_value value)
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
            // invalid or unsupported specifier, do nothing
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
        default:
        case SPECIFIER_SIGNED_DEC:
        case SPECIFIER_UNSIGNED_DEC:
            return 10;
        case SPECIFIER_UNSIGNED_HEX:
        case SPECIFIER_POINTER:
            return 16;
            break;
    }
}
