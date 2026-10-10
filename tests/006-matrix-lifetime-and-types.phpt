--TEST--
Numerus Matrix results own their storage and reject cloning and invalid argument types
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

// The source Matrix is destroyed when the closure returns. The transpose
// remains valid because the PHP façade returns an independently owned result.
$transpose = (static function (): Matrix {
    $source = Matrix::fromRows([[1.0, 2.0], [3.0, 4.0]]);
    return $source->transpose();
})();

gc_collect_cycles();
var_dump($transpose->rows());
var_dump($transpose->columns());
var_dump($transpose->get(0, 1));
var_dump($transpose->get(1, 0));

try {
    clone $transpose;
} catch (Error $e) {
    echo "clone rejected\n";
}

try {
    Matrix::fromRows([1, 2]);
} catch (TypeError $e) {
    echo "non-array row rejected\n";
}

try {
    Matrix::fromRows([[true]]);
} catch (TypeError $e) {
    echo "non-numeric element rejected\n";
}

try {
    $transpose->multiply(new stdClass());
} catch (TypeError $e) {
    echo "invalid matrix operand rejected\n";
}
?>
--EXPECT--
int(2)
int(2)
float(3)
float(2)
clone rejected
non-array row rejected
non-numeric element rejected
invalid matrix operand rejected
