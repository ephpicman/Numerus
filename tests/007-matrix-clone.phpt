--TEST--
Numerus Matrix explicitly rejects cloning
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

$matrix = Matrix::fromRows([[1.0]]);

try {
    clone $matrix;
} catch (Error $e) {
    echo "clone rejected\n";
    return;
}

throw new RuntimeException('Cloning Numerus\\Matrix unexpectedly succeeded');
?>
--EXPECT--
clone rejected
