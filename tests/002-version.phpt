--TEST--
Numerus exposes its module version
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
echo phpversion('numerus'), PHP_EOL;
?>
--EXPECT--
0.1.0
