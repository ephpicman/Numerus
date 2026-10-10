<?php

declare(strict_types=1);

use Numerus\Matrix;

$design = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
]);

$response = Matrix::fromRows([
    [3.0],
    [5.0],
    [7.0],
]);

// Recommended OLS coefficient path: pivoted-QR least squares.
$coefficients = $design->leastSquares($response);

$intercept = $coefficients->get(0, 0);
$slope = $coefficients->get(1, 0);

if (abs($intercept - 1.0) > 1e-10 || abs($slope - 2.0) > 1e-10) {
    throw new RuntimeException('OLS coefficients differ from the expected fixture.');
}

printf("intercept: %.6f\n", $intercept);
printf("slope: %.6f\n", $slope);
