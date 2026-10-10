--TEST--
Numerus Matrix leastSquares exposes its documented named argument
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

$design = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
]);
$response = Matrix::fromRows([[3.0], [5.0], [7.0]]);

$beta = $design->leastSquares(rightHandSide: $response);
var_dump(abs($beta->get(0, 0) - 1.0) < 1e-10);
var_dump(abs($beta->get(1, 0) - 2.0) < 1e-10);

$multiplyParameter = (new ReflectionMethod(Matrix::class, 'multiply'))
    ->getParameters()[0]->getName();
$leastSquaresParameter = (new ReflectionMethod(Matrix::class, 'leastSquares'))
    ->getParameters()[0]->getName();

var_dump($multiplyParameter);
var_dump($leastSquaresParameter);
?>
--EXPECT--
bool(true)
bool(true)
string(5) "other"
string(13) "rightHandSide"
