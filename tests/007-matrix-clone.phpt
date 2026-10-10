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
$cloneRejected = false;

try {
    clone $matrix;
} catch (Error $e) {
    $cloneRejected = true;
}

if (!$cloneRejected) {
    throw new RuntimeException('Cloning Numerus\\Matrix unexpectedly succeeded');
}

echo "clone rejected\n";
?>
--EXPECT--
clone rejected
