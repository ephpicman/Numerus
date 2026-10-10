--TEST--
Numerus Matrix composes the explicit normal-equation OLS path and stable least squares
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

$x = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
]);
$y = Matrix::fromRows([[3.0], [5.0], [7.0]]);

// The explicit formula is supported for users who need to compose it.
$xt = $x->transpose();
$normalEquation = $xt->multiply($x)->inverse()->multiply($xt->multiply($y));

// The direct least-squares primitive is the recommended default.
$qr = $x->leastSquares($y);

var_dump(abs($normalEquation->get(0, 0) - 1.0) < 1e-10);
var_dump(abs($normalEquation->get(1, 0) - 2.0) < 1e-10);
var_dump(abs($qr->get(0, 0) - 1.0) < 1e-10);
var_dump(abs($qr->get(1, 0) - 2.0) < 1e-10);
?>
--EXPECT--
bool(true)
bool(true)
bool(true)
bool(true)
