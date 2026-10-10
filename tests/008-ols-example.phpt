--TEST--
Numerus PHP OLS example runs end to end
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
require __DIR__ . '/../examples/ols.php';
?>
--EXPECT--
intercept: 1.000000
slope: 2.000000
