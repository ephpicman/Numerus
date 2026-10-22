--TEST--
Numerus PHP WLS example runs end to end
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
require __DIR__ . '/../examples/wls.php';
?>
--EXPECT--
intercept: 1.000000
slope: 1.000000
