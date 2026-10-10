--TEST--
Numerus Matrix rejects invalid shapes, coordinates, and incompatible products
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

try {
    Matrix::fromRows([[1, 2], [3]]);
} catch (ValueError $e) {
    echo "ragged rows rejected\n";
}

try {
    Matrix::fromRows([[1], []]);
} catch (ValueError $e) {
    echo "empty row rejected\n";
}

try {
    Matrix::fromRows([[1]])->get(-1, 0);
} catch (ValueError $e) {
    echo "negative coordinate rejected\n";
}

try {
    Matrix::fromRows([[1, 2]])->multiply(Matrix::fromRows([[1, 2]]));
} catch (ValueError $e) {
    echo "incompatible product rejected\n";
}

try {
    Matrix::fromRows([[1, 2], [2, 4]])->inverse();
} catch (Exception $e) {
    echo "singular inverse rejected\n";
}
?>
--EXPECT--
ragged rows rejected
empty row rejected
negative coordinate rejected
incompatible product rejected
singular inverse rejected
