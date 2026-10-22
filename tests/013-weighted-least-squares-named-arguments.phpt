--TEST--
Numerus Matrix weightedLeastSquares exposes documented named arguments
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

$beta = $design->weightedLeastSquares(
    rightHandSide: $response,
    weights: [1.0, 1.0, 1.0],
);
var_dump(abs($beta->get(0, 0) - 1.0) < 1e-10);
var_dump(abs($beta->get(1, 0) - 2.0) < 1e-10);

$parameters = (new ReflectionMethod(Matrix::class, 'weightedLeastSquares'))
    ->getParameters();
var_dump($parameters[0]->getName());
var_dump($parameters[1]->getName());
?>
--EXPECT--
bool(true)
bool(true)
string(13) "rightHandSide"
string(7) "weights"
