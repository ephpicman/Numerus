--TEST--
Numerus extension loads
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
var_dump(extension_loaded('numerus'));
?>
--EXPECT--
bool(true)
