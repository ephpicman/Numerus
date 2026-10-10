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
