--TEST--
Numerus Matrix least-squares handles multiple RHS and documents failure semantics
--SKIPIF--
<?php
if (!extension_loaded('numerus')) {
    die('skip numerus extension is not loaded');
}
?>
--FILE--
<?php
use Numerus\Matrix;

// Multiple RHS: both columns are exact linear combinations of a full-rank design.
$x = Matrix::fromRows([
    [1.0, 0.0],
    [0.0, 1.0],
    [1.0, 1.0],
]);
$multipleY = Matrix::fromRows([
    [1.0, 2.0],
    [2.0, 4.0],
    [3.0, 6.0],
]);
$multipleBeta = $x->leastSquares($multipleY);
var_dump($multipleBeta->rows());
var_dump($multipleBeta->columns());
var_dump(abs($multipleBeta->get(0, 0) - 1.0) < 1e-10);
var_dump(abs($multipleBeta->get(1, 0) - 2.0) < 1e-10);
var_dump(abs($multipleBeta->get(0, 1) - 2.0) < 1e-10);
var_dump(abs($multipleBeta->get(1, 1) - 4.0) < 1e-10);

// Noisy overdetermined system: verify the known least-squares solution and
// normal-equation residual orthogonality without adding a model-specific API.
$noisyY = Matrix::fromRows([[1.0], [2.0], [4.0]]);
$beta = $x->leastSquares($noisyY);
$b0 = $beta->get(0, 0);
$b1 = $beta->get(1, 0);
$r0 = $b0 - 1.0;
$r1 = $b1 - 2.0;
$r2 = $b0 + $b1 - 4.0;
var_dump(abs($b0 - 4.0 / 3.0) < 1e-10);
var_dump(abs($b1 - 7.0 / 3.0) < 1e-10);
var_dump(abs($r0 + $r2) < 1e-10);
var_dump(abs($r1 + $r2) < 1e-10);

// Rank-deficient design matrices are rejected by the QR least-squares API.
try {
    Matrix::fromRows([[1.0, 2.0], [2.0, 4.0]])->leastSquares(
        Matrix::fromRows([[1.0], [2.0]])
    );
} catch (Exception $e) {
    echo get_class($e), ": rank deficient\n";
}

// Underdetermined systems are rejected; users can compose A+ b with the
// existing pseudoinverse C primitive, but it is not exposed by this PHP API yet.
try {
    Matrix::fromRows([[1.0, 1.0]])->leastSquares(Matrix::fromRows([[2.0]]));
} catch (Exception $e) {
    echo get_class($e), ": underdetermined\n";
}

// Row mismatch is an input/shape error and maps to ValueError.
try {
    $x->leastSquares(Matrix::fromRows([[1.0], [2.0]]));
} catch (ValueError $e) {
    echo get_class($e), ": RHS row mismatch\n";
}

// The C solver rejects non-finite values in the response and design matrix.
try {
    $x->leastSquares(Matrix::fromRows([[1.0], [NAN], [3.0]]));
} catch (Exception $e) {
    echo get_class($e), ": non-finite RHS\n";
}
try {
    Matrix::fromRows([[1.0, NAN], [0.0, 1.0], [1.0, 1.0]])
        ->leastSquares(Matrix::fromRows([[1.0], [2.0], [3.0]]));
} catch (Exception $e) {
    echo get_class($e), ": non-finite design\n";
}
?>
--EXPECT--
int(2)
int(2)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
bool(true)
Exception: rank deficient
Exception: underdetermined
ValueError: RHS row mismatch
Exception: non-finite RHS
Exception: non-finite design
