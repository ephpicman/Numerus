<?php

declare(strict_types=1);

use Numerus\\Matrix;

$design = Matrix::fromRows([
    [1.0, 1.0],
    [1.0, 2.0],
    [1.0, 3.0],
    [1.0, 4.0],
]);

$response = Matrix::fromRows([
    [2.0],
    [2.9],
    [4.2],
    [4.8],
]);

// Larger weights give observations more influence in the weighted objective.
$weights = [1.0, 2.0, 2.0, 1.0];
$coefficients = $design->weightedLeastSquares($response, $weights);

$intercept = $coefficients->get(0, 0);
$slope = $coefficients->get(1, 0);

if (abs($intercept - 1.0) > 1e-10 || abs($slope - 1.0) > 1e-10) {
    throw new RuntimeException('WLS coefficients differ from the expected fixture.');
}

printf("intercept: %.6f\\n", $intercept);
printf("slope: %.6f\\n", $slope);
