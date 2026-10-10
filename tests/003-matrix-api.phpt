--TEST--
Numerus Matrix exposes construction, dimensions, and zero-based access
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

$matrix = Matrix::fromRows([[1, 2.5], [3, 4]]);
var_dump($matrix->rows());
var_dump($matrix->columns());
var_dump($matrix->get(0, 1));
var_dump($matrix->get(1, 0));

$zeros = Matrix::zeros(2, 3);
var_dump($zeros->rows());
var_dump($zeros->columns());
var_dump($zeros->get(1, 2));
?>
--EXPECT--
int(2)
int(2)
float(2.5)
float(3)
int(2)
int(3)
float(0)
