/**
 * @file php_numerus.h
 * @brief PHP-facing declarations and version metadata for the Numerus extension.
 *
 * @details This file belongs to Numerus's internal C implementation. Its
 * declarations and behavior are coordinated with the focused headers in the
 * same subsystem; changes should preserve their documented ownership,
 * validation, and error-reporting contracts.
 */

#ifndef PHP_NUMERUS_H
#define PHP_NUMERUS_H

extern zend_module_entry numerus_module_entry;
#define phpext_numerus_ptr &numerus_module_entry

#define PHP_NUMERUS_VERSION "0.1.0"

#endif
