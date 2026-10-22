--TEST--
Numerus weighted least-squares PHP API
--FILE--
<?php
use Numerus\\Matrix;

$x = Matrix::fromRows([[1.0, 0.0], [0.0, 1.0], [1.0, 1.0]]);
$y = Matrix::fromRows([[1.0], [2.0], [4.0]]);
$ols = $x->leastSquares($y);
$wls = $x->weightedLeastSquares($y, [1.0, 1.0, 1.0]);
var_dump(abs($ols->get(0, 0) - $wls->get(0, 0)) < 1e-10);
var_dump(abs($ols->get(1, 0) - $wls->get(1, 0)) < 1e-10);

$x2 = Matrix::fromRows([[1.0, 0.0], [0.0, 1.0], [1.0, 1.0]]);
$y2 = Matrix::fromRows([[1.0], [2.0], [100.0]]);
$zeroWeight = $x2->weightedLeastSquares($y2, [1.0, 1.0, 0.0]);
var_dump(abs($zeroWeight->get(0, 0) - 1.0) < 1e-10);
var_dump(abs($zeroWeight->get(1, 0) - 2.0) < 1e-10);

try {
    $x->weightedLeastSquares($y, [1.0, -1.0, 1.0]);
} catch (ValueError $e) {
    echo "negative weight rejected\n";
}
try {
    $x->weightedLeastSquares($y, [1.0, NAN, 1.0]);
} catch (Exception $e) {
    echo "non-finite weight rejected\n";
}
try {
    $x->weightedLeastSquares($y, [1.0, 1.0]);
} catch (ValueError $e) {
    echo "weight count rejected\n";
}
try {
    $x->weightedLeastSquares($y, [1.0, "2", 1.0]);
} catch (TypeError $e) {
    echo "non-numeric weight rejected\n";
}
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
negative weight rejected
non-finite weight rejected
weight count rejected
non-numeric weight rejected
