/**
 * @file numerus.c
 * @brief PHP extension module entry point and lifecycle hooks.
 *
 * @details This file belongs to Numerus's internal C implementation. Its
 * declarations and behavior are coordinated with the focused headers in the
 * same subsystem; changes should preserve their documented ownership,
 * validation, and error-reporting contracts.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "php.h"
#include "../php_numerus.h"

PHP_MINIT_FUNCTION(numerus)
{
    return SUCCESS;
}

zend_module_entry numerus_module_entry = {
    STANDARD_MODULE_HEADER,
    "numerus",
    NULL,
    PHP_MINIT(numerus),
    NULL,
    NULL,
    NULL,
    NULL,
    PHP_NUMERUS_VERSION,
    STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_NUMERUS
# ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE()
# endif
ZEND_GET_MODULE(numerus)
#endif
